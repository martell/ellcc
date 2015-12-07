
nghttp2_session_get_stream_effective_local_window_size
======================================================

Synopsis
--------

*#include <nghttp2/nghttp2.h>*

.. function:: int32_t nghttp2_session_get_stream_effective_local_window_size(nghttp2_session *session, int32_t stream_id)

    
    Returns the local (receive) window size for the stream *stream_id*.
    The local window size can be adjusted by
    `nghttp2_submit_window_update()`.  This function takes into account
    that and returns effective window size.
    
    This function returns -1 if it fails.
