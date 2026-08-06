// GIF loading: every frame composited into one indexed spritesheet.
//
// A GIF frame is a delta on the frame before it, which suits playing an
// animation forwards and nothing else. Compositing at load turns the file into
// an ordinary spritesheet - frame n is sheet.sprite(n, 0) - for frames * width *
// height bytes, which an 8bpp indexed sheet keeps affordable.
//
// The LZW tables are per-frame state, so like the PNG and JPEG decoders they
// live in the shared working buffer and nothing survives the call. Statuses are
// returned, not raised: image.cpp turns them into exceptions, as it does for the
// other two decoders.

#include "pv_objs.hpp"

extern "C" {

  #include "py/stream.h"
  #include "py/runtime.h"
  #include "extmod/vfs.h"

  // The decoder's tables, then the frame table, in the scratch pool the
  // rasteriser is not using while a file is being read.
  static_assert(sizeof(gif_scratch_t) + sizeof(gif_frame_t) <= PV_WORKING_BUFFER_SIZE,
                "the GIF decoder doesn't fit in the working buffer");
  static_assert(sizeof(gif_scratch_t) % alignof(gif_frame_t) == 0,
                "the frame table would be misaligned after the decoder's tables");

  struct gif_ram_source_t {
    const uint8_t *data;
    size_t size;
    size_t pos;
  };

  static gif_scratch_t *gif_scratch() {
    return (gif_scratch_t *)PicoVector_working_buffer;
  }

  static gif_frame_t *gif_frames() {
    return (gif_frame_t *)(PicoVector_working_buffer + sizeof(gif_scratch_t));
  }

  static int gif_max_frames() {
    return (int)((PV_WORKING_BUFFER_SIZE - sizeof(gif_scratch_t)) / sizeof(gif_frame_t));
  }

  static size_t gifdec_read_callback(void *handle, void *dest, size_t len) {
    gif_handle_t *gif_handle = (gif_handle_t *)handle;
    int error;
    mp_uint_t read = mp_stream_read_exactly(gif_handle->fhandle, dest, len, &error);
    return read == MP_STREAM_ERROR ? 0 : (size_t)read;
  }

  // Re-implementation of stream.c/static mp_obj_t stream_seek(size_t n_args, const mp_obj_t *args)
  static bool gifdec_rewind_callback(void *handle) {
    gif_handle_t *gif_handle = (gif_handle_t *)handle;
    struct mp_stream_seek_t seek_s;
    seek_s.offset = 0;
    seek_s.whence = SEEK_SET;

    const mp_stream_p_t *stream_p = mp_get_stream(gif_handle->fhandle);

    int error;
    mp_uint_t res = stream_p->ioctl(gif_handle->fhandle, MP_STREAM_SEEK,
                                    (mp_uint_t)(uintptr_t)&seek_s, &error);
    return res != MP_STREAM_ERROR;
  }

  static size_t gifdec_ram_read(void *handle, void *dest, size_t len) {
    gif_ram_source_t *source = (gif_ram_source_t *)handle;
    size_t available = source->size - source->pos;
    if(len > available) len = available;
    memcpy(dest, source->data + source->pos, len);
    source->pos += len;
    return len;
  }

  static bool gifdec_ram_rewind(void *handle) {
    ((gif_ram_source_t *)handle)->pos = 0;
    return true;
  }

  // Per-frame durations, in the milliseconds badge.ticks counts.
  static mp_obj_t gifdec_delays(const gif_frame_t *frames, int count) {
    mp_obj_tuple_t *result = (mp_obj_tuple_t *)MP_OBJ_TO_PTR(mp_obj_new_tuple(count, NULL));
    for(int i = 0; i < count; i++) result->items[i] = MP_OBJ_NEW_SMALL_INT(frames[i].delay);
    return MP_OBJ_FROM_PTR(result);
  }

  static int gifdec_open(image_obj_t &target, gif_reader_t reader, gif_info_t &info) {
    gif_frame_t *frames = gif_frames();
    gif_scratch_t *scratch = gif_scratch();

    int status = gif_survey(reader, scratch, &info, frames, gif_max_frames());
    if(status != GIF_OK) return status;

    size_t bytes = (size_t)info.width * info.height * info.frame_count;
    if(bytes > PV_GIF_MAX_BYTES) return GIF_TOO_BIG;

    scratch->restore = nullptr;
    scratch->restore_size = 0;
    if(info.restore_bytes > 0) {
      // Whatever is left of the working buffer past the decoder's tables and the
      // frame table, or the heap when a full-screen frame wants more than that.
      size_t used = sizeof(gif_scratch_t) + (size_t)info.frame_count * sizeof(gif_frame_t);
      size_t spare = PV_WORKING_BUFFER_SIZE - used;
      if(spare >= info.restore_bytes) {
        scratch->restore = (uint8_t *)(PicoVector_working_buffer + used);
        scratch->restore_size = spare;
      } else {
        scratch->restore = (uint8_t *)m_malloc_no_scan(info.restore_bytes);
        scratch->restore_size = info.restore_bytes;
      }
    }

    target.image = new(m_malloc(sizeof(image_t))) image_t(
      (int)info.width * (int)info.frame_count, (int)info.height,
      1, (int)info.frame_count, RGBA8888, true, info.palette_size);

    status = gif_decode(reader, scratch, &info, target.image);
    if(status != GIF_OK) return status;

    target.frame_delays = gifdec_delays(frames, info.frame_count);
    return GIF_OK;
  }

  int gifdec_open_ram(image_obj_t &target, const void* buffer, const size_t size,
                      gif_info_t *info) {
    gif_ram_source_t source = { (const uint8_t *)buffer, size, 0 };
    gif_reader_t reader = { gifdec_ram_read, gifdec_ram_rewind, &source };
    return gifdec_open(target, reader, *info);
  }

  int gifdec_open_file(image_obj_t &target, const char *path, gif_info_t *info) {
    mp_obj_t args[2] = {
      mp_obj_new_str(path, (mp_uint_t)strlen(path)),
      MP_ROM_QSTR(MP_QSTR_r),
    };

    gif_handle_t gif_handle;
    gif_handle.fhandle = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);

    gif_reader_t reader = { gifdec_read_callback, gifdec_rewind_callback, &gif_handle };
    int status = gifdec_open(target, reader, *info);

    mp_stream_close(gif_handle.fhandle);
    return status;
  }
}
