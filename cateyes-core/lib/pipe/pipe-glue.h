#ifndef __CATEYES_PIPE_GLUE_H__
#define __CATEYES_PIPE_GLUE_H__

#include "cateyes-pipe.h"

#define CATEYES_TYPE_WINDOWS_PIPE_INPUT_STREAM (cateyes_windows_pipe_input_stream_get_type ())
#define CATEYES_TYPE_WINDOWS_PIPE_OUTPUT_STREAM (cateyes_windows_pipe_output_stream_get_type ())

G_DECLARE_FINAL_TYPE (CateyesWindowsPipeInputStream, cateyes_windows_pipe_input_stream, CATEYES, WINDOWS_PIPE_INPUT_STREAM, GInputStream)
G_DECLARE_FINAL_TYPE (CateyesWindowsPipeOutputStream, cateyes_windows_pipe_output_stream, CATEYES, WINDOWS_PIPE_OUTPUT_STREAM, GOutputStream)

#endif
