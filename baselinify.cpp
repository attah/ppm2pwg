#include <bytestream.h>
#include <jpeglib.h>

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

  jpeg_mem_src(&srcinfo, InBts.raw(), InBts.size());

  // TODO: consider copying or inserting exif header if applicable

  jpeg_read_header(&srcinfo, TRUE);

  coef_arrays = jpeg_read_coefficients(&srcinfo);

  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  size_t outsize;
  uint8_t* outptr = nullptr;
  jpeg_mem_dest(&dstinfo, &outptr, &outsize);

  jpeg_write_coefficients(&dstinfo, coef_arrays);
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);

  OutBts = Bytestream(outptr, outsize);
}
