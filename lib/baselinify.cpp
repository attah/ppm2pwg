#include "baselinify.h"

#include "madness.h"

#include <jpeglib.h>

#define JPEG_APP1 (JPEG_APP0+1)

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

  static void skip_input_data(j_decompress_ptr cinfo, long numBytes)
  {
    bts_source_mgr* src = (bts_source_mgr*)cinfo->src;
    src->next_input_byte += (size_t)numBytes;
    src->bytes_in_buffer -= (size_t)numBytes;
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

void baselinify(Bytestream& inBts, Bytestream& outBts)
{
  struct jpeg_decompress_struct srcInfo;
  struct jpeg_compress_struct dstInfo;
  struct jpeg_error_mgr jSrcErr, jDstErr;
  jvirt_barray_ptr* coefArrays;

  #if MADNESS
  #include "libfuncs_jpeg"
  #endif

  srcInfo.err = jpeg_std_error(&jSrcErr);
  jpeg_create_decompress(&srcInfo);

  dstInfo.err = jpeg_std_error(&jDstErr);
  jpeg_create_compress(&dstInfo);

  bts_source_mgr srcMgr(inBts);
  srcInfo.src = &srcMgr;

  // Preserve JFIF and EXIF data
  jpeg_save_markers(&srcInfo, JPEG_APP0, 0xFFFF);
  jpeg_save_markers(&srcInfo, JPEG_APP1, 0xFFFF);

  jpeg_read_header(&srcInfo, TRUE);

  coefArrays = jpeg_read_coefficients(&srcInfo);

  jpeg_copy_critical_parameters(&srcInfo, &dstInfo);

  // Don't write automatic JFIF data as it would be done twice
  dstInfo.write_JFIF_header = FALSE;
  dstInfo.write_Adobe_marker = FALSE;

  bts_destination_mgr dstMgr(outBts);
  dstInfo.dest = &dstMgr;

  jpeg_write_coefficients(&dstInfo, coefArrays);

  for(jpeg_saved_marker_ptr marker = srcInfo.marker_list; marker != nullptr; marker = marker->next)
  {
    jpeg_write_marker(&dstInfo, marker->marker, marker->data, marker->data_length);
  }

  jpeg_finish_compress(&dstInfo);
  jpeg_destroy_compress(&dstInfo);
  jpeg_finish_decompress(&srcInfo);
  jpeg_destroy_decompress(&srcInfo);
}
