Bytestream make_pwg_file_hdr();

Bytestream make_urf_file_hdr(uint32_t pages);

void make_pwg_hdr(Bytestream& OutBts, size_t Colors, size_t Quality,
                  size_t HwResX, size_t HwResY, size_t ResX, size_t ResY,
                  bool Duplex, bool Tumble, std::string PageSizeName,
                  bool HFlip, bool VFlip);
void make_urf_hdr(Bytestream& OutBts, size_t Colors, size_t Quality,
                  size_t HwResX, size_t HwResY,size_t ResX, size_t ResY,
                  bool Duplex, bool Tumble);


void bmp_to_pwg(Bytestream& bmp_bts, Bytestream& OutBts, bool Urf,
                size_t page, size_t Colors, size_t Quality,
                size_t HwResX, size_t HwResY, size_t ResX, size_t ResY,
                bool Duplex, bool Tumble, std::string PageSizeName,
                bool BackHFlip, bool BackVFlip);