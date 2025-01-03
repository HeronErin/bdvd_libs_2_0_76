/* sigsetjmp vs alloca test case.  Exercised bug on sparc.  */

#include <stdio.h>
#include <setjmp.h>
#include <alloca.h>

int ret;
int verbose;

static void
sub5 (jmp_buf buf)
{
  siglongjmp (buf, 1);
}

static void
test (int x)
{
  sigjmp_buf buf;
  char *foo;
  int arr[100];

  ++ret;

  arr[77] = x;
  if (sigsetjmp (buf, 1))
    {
      --ret;
      if (verbose)
        printf ("made it ok; %d\n", arr[77]);
      return;
    }

  foo = (char *) alloca (128);
  sub5 (buf);
}

int
main (int argc, char *argv[])
{
  int i;

  verbose = (argc != 1);
  ret = 0;

  for (i = 123; i < 345; ++i)
    test (i);

  return ret;
}
