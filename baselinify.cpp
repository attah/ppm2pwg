#include <bytestream.h>
#include <jpeglib.h>

struct bts_source_mgr
{
  bts_source_mgr(Bytestream& in) : bts(in)
  {
    pub.init_source = init_source;
    pub.fill_input_buffer = fill_input_buffer;
    pub.skip_input_data = skip_input_data;
    pub.resync_to_restart = jpeg_resync_to_restart;
    pub.term_source = term_source;
    pub.bytes_in_buffer = (size_t)bts.size();
    pub.next_input_byte = (const JOCTET *)bts.raw();
  }

  operator jpeg_source_mgr*()
  {
    return (jpeg_source_mgr*)this;
  }

  static void init_source(j_decompress_ptr) {}

  static boolean fill_input_buffer(j_decompress_ptr)
  {
    return TRUE;
  }

  static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
  {
    bts_source_mgr* src = (bts_source_mgr*)cinfo->src;
    src->pub.next_input_byte += (size_t)num_bytes;
    src->pub.bytes_in_buffer -= (size_t)num_bytes;
  }

  static void term_source(j_decompress_ptr) {}

  struct jpeg_source_mgr pub;
  Bytestream& bts;
};

struct bts_destination_mgr
{
  bts_destination_mgr(Bytestream& out) : bts(out)
  {
    pub.init_destination = init_destination;
    pub.empty_output_buffer = empty_output_buffer;
    pub.term_destination = term_destination;
    pub.next_output_byte = buffer;
    pub.free_in_buffer = BS_REASONABLE_FILE_SIZE;
  }

  operator jpeg_destination_mgr*()
  {
    return (jpeg_destination_mgr*)this;
  }

  static void init_destination(j_compress_ptr) {}

  static boolean empty_output_buffer(j_compress_ptr cinfo)
  {
    bts_destination_mgr* dest = (bts_destination_mgr*)cinfo->dest;
    dest->bts.putBytes(dest->buffer, BS_REASONABLE_FILE_SIZE);

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = BS_REASONABLE_FILE_SIZE;
    return TRUE;
  }

  static void term_destination(j_compress_ptr cinfo)
  {
    bts_destination_mgr* dest = (bts_destination_mgr*)cinfo->dest;
    size_t size = (BS_REASONABLE_FILE_SIZE - dest->pub.free_in_buffer);
    dest->bts.putBytes(dest->buffer, size);
  }

  struct jpeg_destination_mgr pub;
  JOCTET buffer[BS_REASONABLE_FILE_SIZE];
  Bytestream& bts;
};

void baselinify(Bytestream& InBts, Bytestream& OutBts)
{
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jsrcerr, jdsterr;
  jvirt_barray_ptr* coef_arrays;

  srcinfo.err = jpeg_std_error(&jsrcerr);
  jpeg_create_decompress(&srcinfo);

  dstinfo.err = jpeg_std_error(&jdsterr);
  jpeg_create_compress(&dstinfo);

  bts_source_mgr srcmgr(InBts);
  srcinfo.src = srcmgr;

  // TODO: consider copying or inserting exif header if applicable

  jpeg_read_header(&srcinfo, TRUE);
  coef_arrays = jpeg_read_coefficients(&srcinfo);
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  bts_destination_mgr dstmgr(OutBts);
  dstinfo.dest = dstmgr;

  jpeg_write_coefficients(&dstinfo, coef_arrays);
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);
}
