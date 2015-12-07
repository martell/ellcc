
nghttp2_hd_deflate_change_table_size
====================================

Synopsis
--------

*#include <nghttp2/nghttp2.h>*

.. function:: int nghttp2_hd_deflate_change_table_size(nghttp2_hd_deflater *deflater, size_t settings_hd_table_bufsize_max)

    
    Changes header table size of the *deflater* to
    *settings_hd_table_bufsize_max* bytes.  This may trigger eviction
    in the dynamic table.
    
    The *settings_hd_table_bufsize_max* should be the value received in
    SETTINGS_HEADER_TABLE_SIZE.
    
    The deflater never uses more memory than
    ``deflate_hd_table_bufsize_max`` bytes specified in
    `nghttp2_hd_deflate_new()`.  Therefore, if
    *settings_hd_table_bufsize_max* > ``deflate_hd_table_bufsize_max``,
    resulting maximum table size becomes
    ``deflate_hd_table_bufsize_max``.
    
    This function returns 0 if it succeeds, or one of the following
    negative error codes:
    
    :macro:`NGHTTP2_ERR_NOMEM`
        Out of memory.
