/*
 * journal.c --- code for handling the "ext3" journal
 *
 * Copyright (C) 2000 Andreas Dilger
 * Copyright (C) 2000 Theodore Ts'o
 *
 * Parts of the code are based on fs/jfs/journal.c by Stephen C. Tweedie
 * Copyright (C) 1999 Red Hat Software
 *
 * This file may be redistributed under the terms of the
 * GNU General Public License version 2 or at your discretion
 * any later version.
 */

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#define MNT_FL (MS_MGC_VAL | MS_RDONLY)
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#define E2FSCK_INCLUDE_INLINE_FUNCS
#include "jfs_user.h"
#include "problem.h"
#include "uuid/uuid.h"

#ifdef CONFIG_JBD_DEBUG		/* Enabled by configure --enable-jfs-debug */
static int bh_count = 0;
int journal_enable_debug = 2;
#endif

/*
 * Define USE_INODE_IO to use the inode_io.c / fileio.c codepaths.
 * This creates a larger static binary, and a smaller binary using
 * shared libraries.  It's also probably slightly less CPU-efficient,
 * which is why it's not on by default.  But, it's a good way of
 * testing the functions in inode_io.c and fileio.c.
 */
#undef USE_INODE_IO

/* Kernel compatibility functions for handling the journal.  These allow us
 * to use the recovery.c file virtually unchanged from the kernel, so we
 * don't have to do much to keep kernel and user recovery in sync.
 */
int journal_bmap(journal_t *journal, blk_t block, unsigned long *phys)
{
#ifdef USE_INODE_IO
	*phys = block;
	return 0;
#else
	struct inode 	*inode = journal->j_inode;
	errcode_t	retval;
	blk_t		pblk;

	if (!inode) {
		*phys = block;
		return 0;
	}

	retval= ext2fs_bmap(inode->i_ctx->fs, inode->i_ino, 
			    &inode->i_ext2, NULL, 0, block, &pblk);
	*phys = pblk;
	return (retval);
#endif
}

struct buffer_head *getblk(kdev_t kdev, blk_t blocknr, int blocksize)
{
	struct buffer_head *bh;

	bh = e2fsck_allocate_memory(kdev->k_ctx, sizeof(*bh), "block buffer");
	if (!bh)
		return NULL;

	jfs_debug(4, "getblk for block %lu (%d bytes)(total %d)\n",
		  (unsigned long) blocknr, blocksize, ++bh_count);

	bh->b_ctx = kdev->k_ctx;
	if (kdev->k_dev == K_DEV_FS)
		bh->b_io = kdev->k_ctx->fs->io;
	else 
		bh->b_io = kdev->k_ctx->journal_io;
	bh->b_size = blocksize;
	bh->b_blocknr = blocknr;

	return bh;
}

void ll_rw_block(int rw, int nr, struct buffer_head *bhp[])
{
	int retval;
	struct buffer_head *bh;

	for (; nr > 0; --nr) {
		bh = *bhp++;
		if (rw == READ && !bh->b_uptodate) {
			jfs_debug(3, "reading block %lu/%p\n", 
				  (unsigned long) bh->b_blocknr, (void *) bh);
			retval = io_channel_read_blk(bh->b_io, 
						     bh->b_blocknr,
						     1, bh->b_data);
			if (retval) {
				com_err(bh->b_ctx->device_name, retval,
					"while reading block %ld\n", 
					bh->b_blocknr);
				bh->b_err = retval;
				continue;
			}
			bh->b_uptodate = 1;
		} else if (rw == WRITE && bh->b_dirty) {
			jfs_debug(3, "writing block %lu/%p\n", 
				  (unsigned long) bh->b_blocknr, (void *) bh);
			retval = io_channel_write_blk(bh->b_io, 
						      bh->b_blocknr,
						      1, bh->b_data);
			if (retval) {
				com_err(bh->b_ctx->device_name, retval,
					"while writing block %ld\n", 
					bh->b_blocknr);
				bh->b_err = retval;
				continue;
			}
			bh->b_dirty = 0;
			bh->b_uptodate = 1;
		} else
			jfs_debug(3, "no-op %s for block %lu\n",
				  rw == READ ? "read" : "write", 
				  (unsigned long) bh->b_blocknr);
	}
}

void mark_buffer_dirty(struct buffer_head *bh)
{
	bh->b_dirty = 1;
}

static void mark_buffer_clean(struct buffer_head * bh)
{
	bh->b_dirty = 0;
}

void brelse(struct buffer_head *bh)
{
	if (bh->b_dirty)
		ll_rw_block(WRITE, 1, &bh);
	jfs_debug(3, "freeing block %lu/%p (total %d)\n",
		  (unsigned long) bh->b_blocknr, (void *) bh, --bh_count);
	ext2fs_free_mem((void **) &bh);
}

int buffer_uptodate(struct buffer_head *bh)
{
	return bh->b_uptodate;
}

void mark_buffer_uptodate(struct buffer_head *bh, int val)
{
	bh->b_uptodate = val;
}

void wait_on_buffer(struct buffer_head *bh)
{
	if (!bh->b_uptodate)
		ll_rw_block(READ, 1, &bh);
}


static void e2fsck_clear_recover(e2fsck_t ctx, int error)
{
	ctx->fs->super->s_feature_incompat &= ~EXT3_FEATURE_INCOMPAT_RECOVER;

	/* if we had an error doing journal recovery, we need a full fsck */
	if (error)
		ctx->fs->super->s_state &= ~EXT2_VALID_FS;
	ext2fs_mark_super_dirty(ctx->fs);
}

static errcode_t e2fsck_get_journal(e2fsck_t ctx, journal_t **ret_journal)
{
	struct ext2_super_block *sb = ctx->fs->super;
	struct ext2_super_block jsuper;
	struct problem_context	pctx;
	struct buffer_head 	*bh;
	struct inode		*j_inode = NULL;
	struct kdev_s		*dev_fs = NULL, *dev_journal;
	const char		*journal_name = 0;
	journal_t		*journal = NULL;
	errcode_t		retval = 0;
	io_manager		io_ptr = 0;
	unsigned long		start = 0;
	int			free_journal_name = 0;
	int			ext_journal = 0;
		
	clear_problem_context(&pctx);
	
	journal = e2fsck_allocate_memory(ctx, sizeof(journal_t), "journal");
	if (!journal) {
		return EXT2_ET_NO_MEMORY;
	}

	dev_fs = e2fsck_allocate_memory(ctx, 2*sizeof(struct kdev_s), "kdev");
	if (!dev_fs) {
		retval = EXT2_ET_NO_MEMORY;
		goto errout;
	}
	dev_journal = dev_fs+1;
	
	dev_fs->k_ctx = dev_journal->k_ctx = ctx;
	dev_fs->k_dev = K_DEV_FS;
	dev_journal->k_dev = K_DEV_JOURNAL;
	
	journal->j_dev = dev_journal;
	journal->j_fs_dev = dev_fs;
	journal->j_inode = NULL;
	journal->j_blocksize = ctx->fs->blocksize;

	if (uuid_is_null(sb->s_journal_uuid)) {
		if (!sb->s_journal_inum)
			return EXT2_ET_BAD_INODE_NUM;
		j_inode = e2fsck_allocate_memory(ctx, sizeof(*j_inode),
						 "journal inode");
		if (!j_inode) {
			retval = EXT2_ET_NO_MEMORY;
			goto errout;
		}

		j_inode->i_ctx = ctx;
		j_inode->i_ino = sb->s_journal_inum;
		
		if ((retval = ext2fs_read_inode(ctx->fs,
						sb->s_journal_inum,
						&j_inode->i_ext2)))
			goto errout;
		if (!j_inode->i_ext2.i_links_count ||
		    !LINUX_S_ISREG(j_inode->i_ext2.i_mode)) {
			retval = EXT2_ET_NO_JOURNAL;
			goto errout;
		}
		if (j_inode->i_ext2.i_size / journal->j_blocksize <
		    JFS_MIN_JOURNAL_BLOCKS) {
			retval = EXT2_ET_JOURNAL_TOO_SMALL;
			goto errout;
		}

		journal->j_maxlen = j_inode->i_ext2.i_size / journal->j_blocksize;

#ifdef USE_INODE_IO
		retval = ext2fs_inode_io_intern(ctx->fs, sb->s_journal_inum,
						&journal_name);
		if (retval)
			goto errout;

		io_ptr = inode_io_manager;
#else
		journal->j_inode = j_inode;
		ctx->journal_io = ctx->fs->io;
		if ((retval = journal_bmap(journal, 0, &start)) != 0)
			goto errout;
#endif
	} else {
		ext_journal = 1;
		journal_name = ctx->journal_name;
		if (!journal_name) {
			journal_name = ext2fs_find_block_device(sb->s_journal_dev);
			free_journal_name = 1;
		}
			
		if (!journal_name) {
			fix_problem(ctx, PR_0_CANT_FIND_JOURNAL, &pctx);
			return EXT2_ET_LOAD_EXT_JOURNAL;
		}
		
		jfs_debug(1, "Using journal file %s\n", journal_name);
		io_ptr = unix_io_manager;
	}

#if 0
	test_io_backing_manager = io_ptr;
	io_ptr = test_io_manager;
#endif
#ifndef USE_INODE_IO
	if (ext_journal)
#endif
		retval = io_ptr->open(journal_name, IO_FLAG_RW,
				      &ctx->journal_io);
	if (free_journal_name)
		free((void *) journal_name);
	if (retval)
		goto errout;

	io_channel_set_blksize(ctx->journal_io, ctx->fs->blocksize);

	if (ext_journal) {
		if (ctx->fs->blocksize == 1024)
			start = 1;
		bh = getblk(dev_journal, start, ctx->fs->blocksize);
		if (!bh) {
			retval = EXT2_ET_NO_MEMORY;
			goto errout;
		}
		ll_rw_block(READ, 1, &bh);
		if ((retval = bh->b_err) != 0)
			goto errout;
		memcpy(&jsuper, start ? bh->b_data :  bh->b_data + 1024,
		       sizeof(jsuper));
		brelse(bh);
#ifdef EXT2FS_ENABLE_SWAPFS
		if (jsuper.s_magic == ext2fs_swab16(EXT2_SUPER_MAGIC)) 
			ext2fs_swap_super(&jsuper);
#endif
		if (jsuper.s_magic != EXT2_SUPER_MAGIC ||
		    !(jsuper.s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)) {
			fix_problem(ctx, PR_0_EXT_JOURNAL_BAD_SUPER, &pctx);
			retval = EXT2_ET_LOAD_EXT_JOURNAL;
			goto errout;
		}
		/* Make sure the journal UUID is correct */
		if (memcmp(jsuper.s_uuid, ctx->fs->super->s_journal_uuid,
			   sizeof(jsuper.s_uuid))) {
			fix_problem(ctx, PR_0_JOURNAL_BAD_UUID, &pctx);
			retval = EXT2_ET_LOAD_EXT_JOURNAL;
			goto errout;
		}
		
		journal->j_maxlen = jsuper.s_blocks_count;
		start++;
	}

	if (!(bh = getblk(dev_journal, start, journal->j_blocksize))) {
		retval = EXT2_ET_NO_MEMORY;
		goto errout;
	}
	
	journal->j_sb_buffer = bh;
	journal->j_superblock = (journal_superblock_t *)bh->b_data;
	
#ifdef USE_INODE_IO
	if (j_inode)
		ext2fs_free_mem((void **)&j_inode);
#endif

	*ret_journal = journal;
	return 0;

errout:
	if (dev_fs)
		ext2fs_free_mem((void **)&dev_fs);
	if (j_inode)
		ext2fs_free_mem((void **)&j_inode);
	if (journal)
		ext2fs_free_mem((void **)&journal);
	return retval;
	
}

static errcode_t e2fsck_journal_fix_bad_inode(e2fsck_t ctx,
					      struct problem_context *pctx)
{
	struct ext2_super_block *sb = ctx->fs->super;
	int recover = ctx->fs->super->s_feature_incompat &
		EXT3_FEATURE_INCOMPAT_RECOVER;
	int has_journal = ctx->fs->super->s_feature_compat &
		EXT3_FEATURE_COMPAT_HAS_JOURNAL;

	if (has_journal || sb->s_journal_inum) {
		/* The journal inode is bogus, remove and force full fsck */
		pctx->ino = sb->s_journal_inum;
		if (fix_problem(ctx, PR_0_JOURNAL_BAD_INODE, pctx)) {
			if (has_journal && sb->s_journal_inum)
				printf("*** ext3 journal has been deleted - "
				       "filesystem is now ext2 only ***\n\n");
			sb->s_feature_compat &= ~EXT3_FEATURE_COMPAT_HAS_JOURNAL;
			sb->s_journal_inum = 0;
			ctx->flags |= E2F_FLAG_JOURNAL_INODE; /* FIXME: todo */
			e2fsck_clear_recover(ctx, 1);
			return 0;
		}
		return EXT2_ET_BAD_INODE_NUM;
	} else if (recover) {
		if (fix_problem(ctx, PR_0_JOURNAL_RECOVER_SET, pctx)) {
			e2fsck_clear_recover(ctx, 1);
			return 0;
		}
		return EXT2_ET_UNSUPP_FEATURE;
	}
	return 0;
}

#define V1_SB_SIZE	0x0024
static void clear_v2_journal_fields(journal_t *journal)
{
	e2fsck_t ctx = journal->j_dev->k_ctx;
	struct problem_context pctx;

	clear_problem_context(&pctx);

	if (!fix_problem(ctx, PR_0_CLEAR_V2_JOURNAL, &pctx))
		return;

	memset(((char *) journal->j_superblock) + V1_SB_SIZE, 0,
	       ctx->fs->blocksize-V1_SB_SIZE);
	mark_buffer_dirty(journal->j_sb_buffer);
}


static errcode_t e2fsck_journal_load(journal_t *journal)
{
	e2fsck_t ctx = journal->j_dev->k_ctx;
	journal_superblock_t *jsb;
	struct buffer_head *jbh = journal->j_sb_buffer;
	struct problem_context pctx;

	clear_problem_context(&pctx);

	ll_rw_block(READ, 1, &jbh);
	if (jbh->b_err) {
		com_err(ctx->device_name, jbh->b_err,
			_("reading journal superblock\n"));
		return jbh->b_err;
	}

	jsb = journal->j_superblock;
	/* If we don't even have JFS_MAGIC, we probably have a wrong inode */
	if (jsb->s_header.h_magic != htonl(JFS_MAGIC_NUMBER))
		return e2fsck_journal_fix_bad_inode(ctx, &pctx);

	switch (ntohl(jsb->s_header.h_blocktype)) {
	case JFS_SUPERBLOCK_V1:
		journal->j_format_version = 1;
		if (jsb->s_feature_compat ||
		    jsb->s_feature_incompat ||
		    jsb->s_feature_ro_compat ||
		    jsb->s_nr_users)
			clear_v2_journal_fields(journal);
		break;
		
	case JFS_SUPERBLOCK_V2:
		journal->j_format_version = 2;
		if (ntohl(jsb->s_nr_users) > 1 &&
		    (ctx->fs->io == ctx->journal_io))
			clear_v2_journal_fields(journal);
		if (ntohl(jsb->s_nr_users) > 1) {
			fix_problem(ctx, PR_0_JOURNAL_UNSUPP_MULTIFS, &pctx);
			return EXT2_ET_JOURNAL_UNSUPP_VERSION;
		}
		break;

	/*
	 * These should never appear in a journal super block, so if
	 * they do, the journal is badly corrupted.
	 */
	case JFS_DESCRIPTOR_BLOCK:
	case JFS_COMMIT_BLOCK:
	case JFS_REVOKE_BLOCK:
		return EXT2_ET_CORRUPT_SUPERBLOCK;
		
	/* If we don't understand the superblock major type, but there
	 * is a magic number, then it is likely to be a new format we
	 * just don't understand, so leave it alone. */
	default:
		return EXT2_ET_JOURNAL_UNSUPP_VERSION;
	}

	if (JFS_HAS_INCOMPAT_FEATURE(journal, ~JFS_KNOWN_INCOMPAT_FEATURES))
		return EXT2_ET_UNSUPP_FEATURE;
	
	if (JFS_HAS_RO_COMPAT_FEATURE(journal, ~JFS_KNOWN_ROCOMPAT_FEATURES))
		return EXT2_ET_RO_UNSUPP_FEATURE;

	/* We have now checked whether we know enough about the journal
	 * format to be able to proceed safely, so any other checks that
	 * fail we should attempt to recover from. */
	if (jsb->s_blocksize != htonl(journal->j_blocksize)) {
		com_err(ctx->program_name, EXT2_ET_CORRUPT_SUPERBLOCK,
			_("%s: no valid journal superblock found\n"),
			ctx->device_name);
		return EXT2_ET_CORRUPT_SUPERBLOCK;
	}

	if (ntohl(jsb->s_maxlen) < journal->j_maxlen)
		journal->j_maxlen = ntohl(jsb->s_maxlen);
	else if (ntohl(jsb->s_maxlen) > journal->j_maxlen) {
		com_err(ctx->program_name, EXT2_ET_CORRUPT_SUPERBLOCK,
			_("%s: journal too short\n"),
			ctx->device_name);
		return EXT2_ET_CORRUPT_SUPERBLOCK;
	}

	journal->j_tail_sequence = ntohl(jsb->s_sequence);
	journal->j_transaction_sequence = journal->j_tail_sequence;
	journal->j_tail = ntohl(jsb->s_start);
	journal->j_first = ntohl(jsb->s_first);
	journal->j_last = ntohl(jsb->s_maxlen);

	return 0;
}

static void e2fsck_journal_reset_super(e2fsck_t ctx, journal_superblock_t *jsb,
				       journal_t *journal)
{
	char *p;
	union {
		uuid_t uuid;
		__u32 val[4];
	} u;
	__u32 new_seq = 0;
	int i;

	/* Leave a valid existing V1 superblock signature alone.
	 * Anything unrecognisable we overwrite with a new V2
	 * signature. */
	
	if (jsb->s_header.h_magic != htonl(JFS_MAGIC_NUMBER) ||
	    jsb->s_header.h_blocktype != htonl(JFS_SUPERBLOCK_V1)) {
		jsb->s_header.h_magic = htonl(JFS_MAGIC_NUMBER);
		jsb->s_header.h_blocktype = htonl(JFS_SUPERBLOCK_V2);
	}

	/* Zero out everything else beyond the superblock header */
	
	p = ((char *) jsb) + sizeof(journal_header_t);
	memset (p, 0, ctx->fs->blocksize-sizeof(journal_header_t));

	jsb->s_blocksize = htonl(ctx->fs->blocksize);
	jsb->s_maxlen = htonl(journal->j_maxlen);
	jsb->s_first = htonl(1);

	/* Initialize the journal sequence number so that there is "no"
	 * chance we will find old "valid" transactions in the journal.
	 * This avoids the need to zero the whole journal (slow to do,
	 * and risky when we are just recovering the filesystem).
	 */
	uuid_generate(u.uuid);
	for (i = 0; i < 4; i ++)
		new_seq ^= u.val[i];
	jsb->s_sequence = htonl(new_seq);

	mark_buffer_dirty(journal->j_sb_buffer);
	ll_rw_block(WRITE, 1, &journal->j_sb_buffer);
}

static errcode_t e2fsck_journal_fix_corrupt_super(e2fsck_t ctx,
						  journal_t *journal,
						  struct problem_context *pctx)
{
	struct ext2_super_block *sb = ctx->fs->super;
	int recover = ctx->fs->super->s_feature_incompat &
		EXT3_FEATURE_INCOMPAT_RECOVER;

	if (sb->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) {
		if (fix_problem(ctx, PR_0_JOURNAL_BAD_SUPER, pctx)) {
			e2fsck_journal_reset_super(ctx, journal->j_superblock,
						   journal);
			journal->j_transaction_sequence = 1;
			e2fsck_clear_recover(ctx, recover);
			return 0;
		}
		return EXT2_ET_CORRUPT_SUPERBLOCK;
	} else if (e2fsck_journal_fix_bad_inode(ctx, pctx))
		return EXT2_ET_CORRUPT_SUPERBLOCK;

	return 0;
}

static void e2fsck_journal_release(e2fsck_t ctx, journal_t *journal,
				   int reset, int drop)
{
	journal_superblock_t *jsb;

	if (drop)
		mark_buffer_clean(journal->j_sb_buffer);
	else if (!(ctx->options & E2F_OPT_READONLY)) {
		jsb = journal->j_superblock;
		jsb->s_sequence = htonl(journal->j_transaction_sequence);
		if (reset)
			jsb->s_start = 0; /* this marks the journal as empty */
		mark_buffer_dirty(journal->j_sb_buffer);
	}
	brelse(journal->j_sb_buffer);

	if (ctx->journal_io) {
		if (ctx->fs && ctx->fs->io != ctx->journal_io)
			io_channel_close(ctx->journal_io);
		ctx->journal_io = 0;
	}
	
#ifndef USE_INODE_IO
	if (journal->j_inode)
		ext2fs_free_mem((void **)&journal->j_inode);
#endif
	if (journal->j_fs_dev)
		ext2fs_free_mem((void **)&journal->j_fs_dev);
	ext2fs_free_mem((void **)&journal);
}

/*
 * This function makes sure that the superblock fields regarding the
 * journal are consistent.
 */
int e2fsck_check_ext3_journal(e2fsck_t ctx)
{
	struct ext2_super_block *sb = ctx->fs->super;
	journal_t *journal;
	int recover = ctx->fs->super->s_feature_incompat &
		EXT3_FEATURE_INCOMPAT_RECOVER;
	struct problem_context pctx;
	problem_t problem;
	int reset = 0, force_fsck = 0;
	int retval;

	/* If we don't have any journal features, don't do anything more */
	if (!(sb->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) &&
	    !recover && sb->s_journal_inum == 0 && sb->s_journal_dev == 0 &&
	    uuid_is_null(sb->s_journal_uuid))
 		return 0;

	clear_problem_context(&pctx);
	pctx.num = sb->s_journal_inum;

	retval = e2fsck_get_journal(ctx, &journal);
	if (retval) {
		if ((retval == EXT2_ET_BAD_INODE_NUM) ||
		    (retval == EXT2_ET_JOURNAL_TOO_SMALL) ||
		    (retval == EXT2_ET_NO_JOURNAL))
			return e2fsck_journal_fix_bad_inode(ctx, &pctx);
		return retval;
	}

	retval = e2fsck_journal_load(journal);
	if (retval) {
		if ((retval == EXT2_ET_CORRUPT_SUPERBLOCK) ||
		    ((retval == EXT2_ET_UNSUPP_FEATURE) &&
		    (!fix_problem(ctx, PR_0_JOURNAL_UNSUPP_INCOMPAT,
				  &pctx))) ||
		    ((retval == EXT2_ET_RO_UNSUPP_FEATURE) &&
		    (!fix_problem(ctx, PR_0_JOURNAL_UNSUPP_ROCOMPAT,
				  &pctx))) ||
		    ((retval == EXT2_ET_JOURNAL_UNSUPP_VERSION) &&
		    (!fix_problem(ctx, PR_0_JOURNAL_UNSUPP_VERSION, &pctx))))
			retval = e2fsck_journal_fix_corrupt_super(ctx, journal,
								  &pctx);
		e2fsck_journal_release(ctx, journal, 0, 1);
		return retval;
	}

	/*
	 * We want to make the flags consistent here.  We will not leave with
	 * needs_recovery set but has_journal clear.  We can't get in a loop
	 * with -y, -n, or -p, only if a user isn't making up their mind.
	 */
no_has_journal:
	if (!(sb->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL)) {
		recover = sb->s_feature_incompat & EXT3_FEATURE_INCOMPAT_RECOVER;
		pctx.str = "inode";
		if (fix_problem(ctx, PR_0_JOURNAL_HAS_JOURNAL, &pctx)) {
			if (recover &&
			    !fix_problem(ctx, PR_0_JOURNAL_RECOVER_SET, &pctx))
				goto no_has_journal;
			/*
			 * Need a full fsck if we are releasing a
			 * journal stored on a reserved inode.
			 */
			force_fsck = recover ||
				(sb->s_journal_inum < EXT2_FIRST_INODE(sb));
			/* Clear all of the journal fields */
			sb->s_journal_inum = 0;
			sb->s_journal_dev = 0;
			memset(sb->s_journal_uuid, 0,
			       sizeof(sb->s_journal_uuid));
			e2fsck_clear_recover(ctx, force_fsck);
		} else if (!(ctx->options & E2F_OPT_READONLY)) {
			sb->s_feature_compat |= EXT3_FEATURE_COMPAT_HAS_JOURNAL;
			ext2fs_mark_super_dirty(ctx->fs);
		}
	}

	if (sb->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL &&
	    !(sb->s_feature_incompat & EXT3_FEATURE_INCOMPAT_RECOVER) &&
	    journal->j_superblock->s_start != 0) {
		/* Print status information */
		fix_problem(ctx, PR_0_JOURNAL_RECOVERY_CLEAR, &pctx);
		if (ctx->superblock)
			problem = PR_0_JOURNAL_RUN_DEFAULT;
		else
			problem = PR_0_JOURNAL_RUN;
		if (fix_problem(ctx, problem, &pctx)) {
			ctx->options |= E2F_OPT_FORCE;
			sb->s_feature_incompat |=
				EXT3_FEATURE_INCOMPAT_RECOVER;
			ext2fs_mark_super_dirty(ctx->fs);
		} else if (fix_problem(ctx,
				       PR_0_JOURNAL_RESET_JOURNAL, &pctx)) {
			reset = 1;
			sb->s_state &= ~EXT2_VALID_FS;
			ext2fs_mark_super_dirty(ctx->fs);
		}
		/*
		 * If the user answers no to the above question, we
		 * ignore the fact that journal apparently has data;
		 * accidentally replaying over valid data would be far
		 * worse than skipping a questionable recovery.
		 * 
		 * XXX should we abort with a fatal error here?  What
		 * will the ext3 kernel code do if a filesystem with
		 * !NEEDS_RECOVERY but with a non-zero
		 * journal->j_superblock->s_start is mounted?
		 */
	}

	e2fsck_journal_release(ctx, journal, reset, 0);
	return retval;
}

static errcode_t recover_ext3_journal(e2fsck_t ctx)
{
	journal_t *journal;
	int retval;

	journal_init_revoke_caches();
	retval = e2fsck_get_journal(ctx, &journal);
	if (retval)
		return retval;

	retval = e2fsck_journal_load(journal);
	if (retval)
		goto errout;

	retval = journal_init_revoke(journal, 1024);
	if (retval)
		goto errout;
	
	retval = -journal_recover(journal);
	if (retval)
		goto errout;
	
	if (journal->j_superblock->s_errno) {
		ctx->fs->super->s_state |= EXT2_ERROR_FS;
		ext2fs_mark_super_dirty(ctx->fs);
		journal->j_superblock->s_errno = 0;
		mark_buffer_dirty(journal->j_sb_buffer);
	}
		
errout:
	journal_destroy_revoke(journal);
	journal_destroy_revoke_caches();
	e2fsck_journal_release(ctx, journal, 1, 0);
	return retval;
}

int e2fsck_run_ext3_journal(e2fsck_t ctx)
{
	io_manager io_ptr = ctx->fs->io->manager;
	int blocksize = ctx->fs->blocksize;
	errcode_t	retval, recover_retval;

	printf(_("%s: recovering journal\n"), ctx->device_name);
	if (ctx->options & E2F_OPT_READONLY) {
		printf(_("%s: won't do journal recovery while read-only\n"),
		       ctx->device_name);
		return EXT2_ET_FILE_RO;
	}

	if (ctx->fs->flags & EXT2_FLAG_DIRTY)
		ext2fs_flush(ctx->fs);	/* Force out any modifications */

	recover_retval = recover_ext3_journal(ctx);
	
	/*
	 * Reload the filesystem context to get up-to-date data from disk
	 * because journal recovery will change the filesystem under us.
	 */
	ext2fs_close(ctx->fs);
	retval = ext2fs_open(ctx->filesystem_name, EXT2_FLAG_RW,
			     ctx->superblock, blocksize, io_ptr,
			     &ctx->fs);

	if (retval) {
		com_err(ctx->program_name, retval,
			_("while trying to re-open %s"),
			ctx->device_name);
		fatal_error(ctx, 0);
	}
	ctx->fs->priv_data = ctx;

	/* Set the superblock flags */
	e2fsck_clear_recover(ctx, recover_retval);
	return recover_retval;
}

/*
 * This function will move the journal inode from a visible file in
 * the filesystem directory hierarchy to the reserved inode if necessary.
 */
static const char * const journal_names[] = {
	".journal", "journal", ".journal.dat", "journal.dat", 0 };

void e2fsck_move_ext3_journal(e2fsck_t ctx)
{
	struct ext2_super_block *sb = ctx->fs->super;
	struct problem_context	pctx;
	struct ext2_inode 	inode;
	ext2_filsys		fs = ctx->fs;
	ext2_ino_t		ino;
	errcode_t		retval;
	const char * const *	cpp;
	int			group, mount_flags;
	
	/*
	 * If the filesystem is opened read-only, or there is no
	 * journal, or the journal is already in the hidden inode,
	 * then do nothing.
	 */
	if ((ctx->options & E2F_OPT_READONLY) ||
	    (sb->s_journal_inum == 0) ||
	    (sb->s_journal_inum == EXT2_JOURNAL_INO) ||
	    !(sb->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL))
		return;

	/*
	 * If the filesystem is mounted, or we can't tell whether
	 * or not it's mounted, do nothing.
	 */
	retval = ext2fs_check_if_mounted(ctx->filesystem_name, &mount_flags);
	if (retval || (mount_flags & EXT2_MF_MOUNTED))
		return;

	/*
	 * If we can't find the name of the journal inode, then do
	 * nothing.
	 */
	for (cpp = journal_names; *cpp; cpp++) {
		retval = ext2fs_lookup(fs, EXT2_ROOT_INO, *cpp,
				       strlen(*cpp), 0, &ino);
		if ((retval == 0) && (ino == sb->s_journal_inum))
			break;
	}
	if (*cpp == 0)
		return;

	/*
	 * The inode had better have only one link and not be readable.
	 */
	if (ext2fs_read_inode(fs, ino, &inode) != 0)
		return;
	if (inode.i_links_count != 1)
		return;

	/* We need the inode bitmap to be loaded */
	retval = ext2fs_read_bitmaps(fs);
	if (retval)
		return;

	clear_problem_context(&pctx);
	pctx.str = *cpp;
	if (!fix_problem(ctx, PR_0_MOVE_JOURNAL, &pctx))
		return;
		
	/*
	 * OK, we've done all the checks, let's actually move the
	 * journal inode.  Errors at this point mean we need to force
	 * an ext2 filesystem check.
	 */
	if ((retval = ext2fs_unlink(fs, EXT2_ROOT_INO, *cpp, ino, 0)) != 0)
		goto err_out;
	if ((retval = ext2fs_write_inode(fs, EXT2_JOURNAL_INO, &inode)) != 0)
		goto err_out;
	sb->s_journal_inum = EXT2_JOURNAL_INO;
	ext2fs_mark_super_dirty(fs);
	inode.i_links_count = 0;
	inode.i_dtime = time(0);
	if ((retval = ext2fs_write_inode(fs, ino, &inode)) != 0)
		goto err_out;

	group = ext2fs_group_of_ino(fs, ino);
	ext2fs_unmark_inode_bitmap(fs->inode_map, ino);
	ext2fs_mark_ib_dirty(fs);
	fs->group_desc[group].bg_free_inodes_count++;
	fs->super->s_free_inodes_count++;
	return;

err_out:
	pctx.errcode = retval;
	fix_problem(ctx, PR_0_ERR_MOVE_JOURNAL, &pctx);
	fs->super->s_state &= ~EXT2_VALID_FS;
	ext2fs_mark_super_dirty(fs);
	return;
}

