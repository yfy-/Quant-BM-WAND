#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

std::string stem(const std::string& term)
{

  std::string st_term = term;
  std::string ies = "ies";
  std::string es = "es";
  std::string s = "s";

  std::size_t term_len = term.length();
  std::size_t ies_len = ies.length();
  std::size_t es_len = es.length();
  std::size_t s_len = s.length();

  if (term_len >= ies_len &&
      term.compare(term_len - ies_len, ies_len, ies) == 0)
    st_term.replace(term_len - ies_len, ies_len, "y");
  else if (term_len >= es_len &&
           term.compare(term_len - es_len, es_len, es) == 0)
    st_term.replace(term_len - es_len, es_len, "");
  else if (term_len >= s_len && term.compare(term_len - s_len, s_len, s) == 0)
    st_term.replace(term_len - s_len, s_len, "");

  return st_term;

  // if (strcmp(destination + len - 3, "ies") == 0)
  // {
  //   strcpy(destination + len - 3, "y");
  //   len -= 3;
  // }
  // else if (strcmp(destination + len - 2, "es") == 0)
  // {
  //   *(destination + len - 2) = '\0';
  //   len -= 2;
  // }
  // else if (destination[len - 1] == 's')
  // {
  //   *(destination + len - 1) = '\0';
  //   len--;
  // }
}

std::string stem_qline(std::string qline) {
  std::size_t qstart = qline.find(";");
  std::string stem_qline = qline.substr(0, qstart) + ";";
  std::string query = qline.substr(qstart + 1);


  std::size_t st_pos = 0;
  std::size_t spc_pos = 0;

  do {
    spc_pos = query.find(" ", st_pos);
    std::string term = query.substr(st_pos, spc_pos - st_pos);

    stem_qline += stem(term) + " ";
    st_pos = spc_pos + 1;

  } while (spc_pos != std::string::npos);

  stem_qline.replace(stem_qline.length() - 1, 1, "");

  return stem_qline;
}

int main(int argc, char* argv[]) {
  if (argc != 2)
    std::cout << "usage: " << argv[0] << "query_file\n";

  std::string in_fname = std::string(argv[1]);
  std::ifstream in_qfile(in_fname);
  std::vector<std::string> in_qlines;
  std::string qline;

  if (in_qfile.is_open()) {
    while (std::getline(in_qfile, qline))
      in_qlines.push_back(qline);
  } else {
    std::cerr << "Cannot open input file " << in_fname << "\n";
    return -1;
  }

  std::vector<std::string> out_qlines;

  out_qlines.resize(in_qlines.size());
  std::transform(in_qlines.begin(), in_qlines.end(), out_qlines.begin(),
                 stem_qline);


  std::size_t base_pos = in_fname.rfind("/");
  std::string dirname = in_fname.substr(0, base_pos + 1);
  std::string basename = in_fname.substr(base_pos + 1);
  std::size_t ext_pos = basename.rfind(".");
  std::string fname = basename.substr(0, ext_pos);
  std::string ext = basename.substr(ext_pos + 1);
  std::string out_fname = dirname + fname + "_stem." + ext;

  std::ofstream out_qfile(out_fname);

  if (out_qfile.is_open()) {
    for (const auto& qline : out_qlines)
      out_qfile << qline << "\n";
  } else {
    std::cerr << "Cannot open output file " << out_fname << "\n";
    return -1;
  }

  return 0;
}
