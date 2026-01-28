#include "dict.hpp"
#include <cstring>
#include <unordered_map>

// Static dictionary with common patterns (inspired by brotli's approach)
// Contains: common English words, HTML/XML tags, CSS properties, common text patterns
// This helps PPM learn patterns immediately for small files

static const char* DICT_STRINGS[] = {
  // Most common English words (sorted by frequency)
  "the ", "The ", " the ", " and ", "and ", " of ", " to ", " in ", " is ",
  "that ", " that", " for ", "was ", " was", " on ", " with ", "his ", "they ",
  "are ", " are", " be ", " at ", " one ", "have ", " have", "this ", " this",
  "from ", " from", " or ", " had ", "had ", " by ", " not ", "but ", " but",
  "what ", " what", "all ", " all", "were ", " were", "when ", " when",
  "your ", " your", "can ", " can", "said ", " said", "there ", " there",
  "use ", " use", "each ", " each", "which ", " which", "she ", " she",
  "how ", " how", "their ", " their", "will ", " will", "other ", " other",
  "about ", " about", "out ", " out", "many ", " many", "then ", " then",
  "them ", " them", "these ", " these", "some ", " some", "her ", " her",
  "would ", " would", "make ", " make", "like ", " like", "into ", " into",
  "has ", " has", "two ", " two", "more ", " more", "write ", " write",
  "see ", " see", "number ", " number", "way ", " way", "could ", " could",
  "people ", " people", "than ", " than", "first ", " first", "been ", " been",
  "call ", " call", "who ", " who", "its ", " its", "now ", " now",
  "find ", " find", "long ", " long", "down ", " down", "day ", " day",
  "did ", " did", "get ", " get", "come ", " come", "made ", " made",
  "may ", " may", "part ", " part",

  // Common word endings
  "tion ", "tion.", "tion,", "tions ", "ing ", "ing.", "ing,", "ings ",
  "ment ", "ment.", "ment,", "ments ", "able ", "ible ", "ness ", "less ",
  "ful ", "ous ", "ive ", "ed ", "ed.", "ed,", "ly ", "ly.", "ly,",
  "er ", "er.", "er,", "ers ", "est ", "al ", "al.", "al,",

  // HTML/XML common patterns
  "<!DOCTYPE html>", "<!DOCTYPE ", "<html>", "</html>", "<head>", "</head>",
  "<body>", "</body>", "<div>", "</div>", "<span>", "</span>",
  "<p>", "</p>", "<a ", "</a>", "<img ", "<br>", "<br/>", "<hr>",
  "<ul>", "</ul>", "<ol>", "</ol>", "<li>", "</li>",
  "<table>", "</table>", "<tr>", "</tr>", "<td>", "</td>", "<th>", "</th>",
  "<form>", "</form>", "<input ", "<button>", "</button>",
  "<script>", "</script>", "<style>", "</style>", "<link ", "<meta ",
  "<title>", "</title>", "<header>", "</header>", "<footer>", "</footer>",
  "<nav>", "</nav>", "<section>", "</section>", "<article>", "</article>",
  "<h1>", "</h1>", "<h2>", "</h2>", "<h3>", "</h3>",

  // HTML attributes
  " class=\"", " id=\"", " href=\"", " src=\"", " style=\"", " type=\"",
  " name=\"", " value=\"", " alt=\"", " title=\"", " width=\"", " height=\"",
  " rel=\"", " target=\"", " data-", " aria-", " onclick=\"", " onload=\"",

  // CSS properties
  "font-family:", "font-size:", "font-weight:", "color:", "background:",
  "background-color:", "margin:", "margin-top:", "margin-bottom:",
  "margin-left:", "margin-right:", "padding:", "padding-top:",
  "padding-bottom:", "padding-left:", "padding-right:", "border:",
  "border-radius:", "display:", "position:", "width:", "height:",
  "max-width:", "min-width:", "text-align:", "line-height:", "float:",
  "clear:", "overflow:", "z-index:", "opacity:", "transform:",

  // Common CSS values
  ": 0;", ": 0px;", ": auto;", ": none;", ": block;", ": inline;",
  ": inline-block;", ": flex;", ": relative;", ": absolute;", ": fixed;",
  "px;", "em;", "rem;", "%;", "vh;", "vw;",

  // JavaScript patterns
  "function ", "function(", "return ", "return;", "var ", "let ", "const ",
  "if (", "if(", "else {", "else{", "else if", "for (", "for(",
  "while (", "while(", "switch (", "switch(", "case ", "break;",
  "continue;", "null", "undefined", "true", "false", "this.",
  "document.", "window.", "console.log", ".length", ".push(",
  ".forEach(", ".map(", ".filter(", ".reduce(", "=>", "===", "!==",

  // JSON patterns
  "\":", "\": ", "\",", "\": \"", "\"}", "\": {", "\": [", "],",
  "null,", "true,", "false,", "null}", "true}", "false}",

  // XML/namespace patterns
  "<?xml ", "version=\"", "encoding=\"", "xmlns:", "xmlns=\"",
  "<![CDATA[", "]]>", "<!--", "-->",

  // Common punctuation sequences
  ". ", ", ", "; ", ": ", "? ", "! ", "...", " - ", " – ", " — ",
  "(", ")", "[", "]", "{", "}", "\"", "'", "`",
  "\r\n", "\n\n", "  ", "    ", "\t",

  // Numbers
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
  "10", "20", "100", "1000", "2000", "2024", "2025", "2026",

  // Programming common
  "#include ", "#define ", "#ifdef ", "#ifndef ", "#endif", "#pragma ",
  "public ", "private ", "protected ", "static ", "virtual ", "override ",
  "class ", "struct ", "enum ", "typedef ", "template ", "typename ",
  "namespace ", "using ", "new ", "delete ", "void ", "int ", "char ",
  "bool ", "float ", "double ", "string ", "vector", "map", "set",
  "std::", "nullptr", "sizeof(", "static_cast<", "dynamic_cast<",

  // File extensions in paths
  ".html", ".htm", ".css", ".js", ".json", ".xml", ".txt", ".md",
  ".png", ".jpg", ".jpeg", ".gif", ".svg", ".pdf", ".zip",
  ".cpp", ".hpp", ".c", ".h", ".py", ".java", ".go", ".rs",

  // URL patterns
  "http://", "https://", "www.", ".com", ".org", ".net", ".io",
  "/index", "/api/", "/v1/", "/v2/",

  // Common abbreviations
  "e.g.", "i.e.", "etc.", "vs.", "Dr.", "Mr.", "Mrs.", "Ms.",

  // PDF patterns
  "%PDF-", "endobj", "endstream", "stream", " obj\n<<", ">> \n",
  " /Type /", " /Pages ", " /Kids [", " /Count ", " /Parent ",
  " /MediaBox [", " /Contents ", " /Length ", " 0 R", " 0 R >>",
  " 0 R]\n", "trailer", "startxref", "%%EOF", "xref\n",
  "0000000", " 65535 f", " 00000 n", "/Catalog", "/Page",

  nullptr
};

// Build the static dictionary once
static std::vector<uint8_t> BuildDict() {
  std::vector<uint8_t> dict;
  dict.reserve(8192);

  for (int i = 0; DICT_STRINGS[i] != nullptr; i++) {
    const char* s = DICT_STRINGS[i];
    while (*s) {
      dict.push_back((uint8_t)*s++);
    }
  }

  return dict;
}

const std::vector<uint8_t>& GetStaticDict() {
  static std::vector<uint8_t> dict = BuildDict();
  return dict;
}

// LZ77 with dictionary - matches can reference either dictionary or window
// Format:
//   Literal: byte (if < 0xF0)
//   Escape: 0xF0 followed by literal byte (for bytes >= 0xF0)
//   Dict match: 0xF1 len offset_hi offset_lo (match from dictionary)
//   Window match: 0xF2 len offset_hi offset_lo (match from recent window)

constexpr uint8_t DICT_ESC_LIT = 0xF0;
constexpr uint8_t DICT_ESC_DICT = 0xF1;
constexpr uint8_t DICT_ESC_WIN = 0xF2;
constexpr int DICT_MIN_MATCH = 3;
constexpr int DICT_MAX_MATCH = 255 + DICT_MIN_MATCH;
constexpr int DICT_WINDOW = 32768;

// Build a hash table for the dictionary
static std::unordered_map<uint32_t, std::vector<size_t>> BuildDictHash() {
  std::unordered_map<uint32_t, std::vector<size_t>> hash;
  const auto& dict = GetStaticDict();

  for (size_t i = 0; i + 3 < dict.size(); i++) {
    uint32_t h = ((uint32_t)dict[i] << 16) | ((uint32_t)dict[i+1] << 8) | dict[i+2];
    hash[h].push_back(i);
  }
  return hash;
}

static const std::unordered_map<uint32_t, std::vector<size_t>>& GetDictHash() {
  static auto hash = BuildDictHash();
  return hash;
}

std::vector<uint8_t> DictEncode(const std::vector<uint8_t> &in) {
  if (in.empty()) return in;

  const auto& dict = GetStaticDict();
  const auto& dict_hash = GetDictHash();

  std::vector<uint8_t> out;
  out.reserve(in.size());

  // Window hash for recent matches
  std::unordered_map<uint32_t, std::vector<size_t>> win_hash;

  for (size_t i = 0; i < in.size();) {
    int best_len = 0;
    size_t best_off = 0;
    bool from_dict = false;

    if (i + 2 < in.size()) {
      uint32_t h = ((uint32_t)in[i] << 16) | ((uint32_t)in[i+1] << 8) | in[i+2];

      // Try dictionary matches
      auto dit = dict_hash.find(h);
      if (dit != dict_hash.end()) {
        for (size_t pos : dit->second) {
          int len = 0;
          while (i + len < in.size() && pos + len < dict.size() &&
                 len < DICT_MAX_MATCH && dict[pos + len] == in[i + len]) {
            len++;
          }
          if (len >= DICT_MIN_MATCH && len > best_len) {
            best_len = len;
            best_off = pos;
            from_dict = true;
          }
        }
      }

      // Try window matches
      auto wit = win_hash.find(h);
      if (wit != win_hash.end()) {
        for (auto pit = wit->second.rbegin(); pit != wit->second.rend(); ++pit) {
          size_t pos = *pit;
          if (i - pos > DICT_WINDOW) break;

          int len = 0;
          while (i + len < in.size() && len < DICT_MAX_MATCH &&
                 in[pos + len] == in[i + len]) {
            len++;
          }
          // Window match needs to be better than dict match (accounts for same encoding cost)
          if (len >= DICT_MIN_MATCH && len > best_len) {
            best_len = len;
            best_off = i - pos;
            from_dict = false;
          }
        }
      }
    }

    if (best_len >= DICT_MIN_MATCH) {
      // Emit match
      if (from_dict) {
        out.push_back(DICT_ESC_DICT);
      } else {
        out.push_back(DICT_ESC_WIN);
      }
      out.push_back((uint8_t)(best_len - DICT_MIN_MATCH));
      out.push_back((uint8_t)(best_off >> 8));
      out.push_back((uint8_t)(best_off & 0xFF));

      // Update window hash
      for (int j = 0; j < best_len && i + j + 2 < in.size(); j++) {
        uint32_t h = ((uint32_t)in[i+j] << 16) | ((uint32_t)in[i+j+1] << 8) | in[i+j+2];
        win_hash[h].push_back(i + j);
        if (win_hash[h].size() > 64) win_hash[h].erase(win_hash[h].begin());
      }
      i += best_len;
    } else {
      // Emit literal
      if (in[i] >= DICT_ESC_LIT) {
        out.push_back(DICT_ESC_LIT);
      }
      out.push_back(in[i]);

      // Update window hash
      if (i + 2 < in.size()) {
        uint32_t h = ((uint32_t)in[i] << 16) | ((uint32_t)in[i+1] << 8) | in[i+2];
        win_hash[h].push_back(i);
        if (win_hash[h].size() > 64) win_hash[h].erase(win_hash[h].begin());
      }
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> DictDecode(const std::vector<uint8_t> &in) {
  const auto& dict = GetStaticDict();

  std::vector<uint8_t> out;
  out.reserve(in.size() * 2);

  for (size_t i = 0; i < in.size();) {
    if (in[i] == DICT_ESC_LIT) {
      // Escaped literal
      if (i + 1 >= in.size()) break;
      out.push_back(in[i + 1]);
      i += 2;
    } else if (in[i] == DICT_ESC_DICT) {
      // Dictionary match
      if (i + 3 >= in.size()) break;
      int len = in[i + 1] + DICT_MIN_MATCH;
      size_t off = ((size_t)in[i + 2] << 8) | in[i + 3];

      for (int j = 0; j < len && off + j < dict.size(); j++) {
        out.push_back(dict[off + j]);
      }
      i += 4;
    } else if (in[i] == DICT_ESC_WIN) {
      // Window match
      if (i + 3 >= in.size()) break;
      int len = in[i + 1] + DICT_MIN_MATCH;
      size_t off = ((size_t)in[i + 2] << 8) | in[i + 3];

      if (off > out.size()) break;
      size_t pos = out.size() - off;
      for (int j = 0; j < len; j++) {
        out.push_back(out[pos + j]);
      }
      i += 4;
    } else {
      // Literal
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}
