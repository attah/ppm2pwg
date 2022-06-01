#include <bytestream.h>
#include <jpeglib.h>
#include "madness.h"

struct bts_source_mgr: jpeg_source_mgr
{
  bts_source_mgr(Bytestream& in) : bts(in)
  {
    #if MADNESS
    LIB(jpeg, "libjpeg.so.62");
    FUNC(jpeg, boolean, jpeg_resync_to_restart, j_decompress_ptr, int);
    #endif

    jpeg_source_mgr::init_source = init_source;
    jpeg_source_mgr::fill_input_buffer = fill_input_buffer;
    jpeg_source_mgr::skip_input_data = skip_input_data;
    jpeg_source_mgr::resync_to_restart = jpeg_resync_to_restart;
    jpeg_source_mgr::term_source = term_source;
    jpeg_source_mgr::bytes_in_buffer = (size_t)bts.size();
    jpeg_source_mgr::next_input_byte = (const JOCTET *)bts.raw();
  }

  static void init_source(j_decompress_ptr) {}

  static boolean fill_input_buffer(j_decompress_ptr)
  {
    return TRUE;
  }

  static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
  {
    bts_source_mgr* src = (bts_source_mgr*)cinfo->src;
    src->next_input_byte += (size_t)num_bytes;
    src->bytes_in_buffer -= (size_t)num_bytes;
  }

  static void term_source(j_decompress_ptr) {}

  Bytestream& bts;
};

struct bts_destination_mgr: jpeg_destination_mgr
{
  bts_destination_mgr(Bytestream& out) : bts(out)
  {
    jpeg_destination_mgr::init_destination = init_destination;
    jpeg_destination_mgr::empty_output_buffer = empty_output_buffer;
    jpeg_destination_mgr::term_destination = term_destination;
    jpeg_destination_mgr::next_output_byte = buffer;
    jpeg_destination_mgr::free_in_buffer = BS_REASONABLE_FILE_SIZE;
  }

  static void init_destination(j_compress_ptr) {}

  static boolean empty_output_buffer(j_compress_ptr cinfo)
  {
    bts_destination_mgr* dest = (bts_destination_mgr*)cinfo->dest;
    dest->bts.putBytes(dest->buffer, BS_REASONABLE_FILE_SIZE);

    dest->next_output_byte = dest->buffer;
    dest->free_in_buffer = BS_REASONABLE_FILE_SIZE;
    return TRUE;
  }

  static void term_destination(j_compress_ptr cinfo)
  {
    bts_destination_mgr* dest = (bts_destination_mgr*)cinfo->dest;
    size_t size = (BS_REASONABLE_FILE_SIZE - dest->free_in_buffer);
    dest->bts.putBytes(dest->buffer, size);
  }

  JOCTET buffer[BS_REASONABLE_FILE_SIZE];
  Bytestream& bts;
};

void baselinify(Bytestream& InBts, Bytestream& OutBts)
{
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jsrcerr, jdsterr;
  jvirt_barray_ptr* coef_arrays;

  #if MADNESS
  #include "libfuncs_jpeg"
  #endif

  srcinfo.err = jpeg_std_error(&jsrcerr);
  jpeg_create_decompress(&srcinfo);

  dstinfo.err = jpeg_std_error(&jdsterr);
  jpeg_create_compress(&dstinfo);

  bts_source_mgr srcmgr(InBts);
  srcinfo.src = &srcmgr;

  // TODO: consider copying or inserting exif header if applicable

  jpeg_read_header(&srcinfo, TRUE);
  coef_arrays = jpeg_read_coefficients(&srcinfo);
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  bts_destination_mgr dstmgr(OutBts);
  dstinfo.dest = &dstmgr;

  jpeg_write_coefficients(&dstinfo, coef_arrays);
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);
}
