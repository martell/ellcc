/* connector for getpid */

#include <reent.h>

int getpid(void)
{
  return _getpid_r(_REENT);
}
