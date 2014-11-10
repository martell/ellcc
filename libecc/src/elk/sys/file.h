/** File definitions.
 */
#ifndef _file_h_
#define _file_h_

typedef struct fd_set *fdset_t;

/** Release a set of file descriptors.
 */
void __elk_fdset_release(fdset_t *set);

#endif
