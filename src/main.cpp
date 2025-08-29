#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

struct Buffer {
  string name;
  vector<string> lines;
  string file_path;
  bool is_modified;

  Buffer(string buff_name) : name(buff_name), is_modified(false) {}
};

class BufferManager {
private:
  map<string, Buffer *> buffers;
  Buffer *current_buffer;
  string temp_directory;

public:
  BufferManager();
  ~BufferManager();

  // Buffer management
  Buffer *create_buffer(string name);
  Buffer *get_buffer(string name);
  bool select_buffer(string name);
  void save_buffer_to_temp(Buffer *buf);
  void load_buffer_from_temp(string name);

  // Buffer operations
  bool open_file(string buffer_name, string file_path);
  bool save_file(string buffer_name, string file_path = "");
  bool create_new_buffer(string buffer_name, string file_path = "");
  void print_buffer(string buffer_name);
  void append_to_buffer(string buffer_name, string content);

  // Line operations
  bool replace_line(string buffer_name, int line_num, string content);
  bool insert_line(string buffer_name, int line_num, string content);
  bool delete_line(string buffer_name, int line_num);
  bool move_line(string buffer_name, int from_line_num, int to_line_num);
  bool copy_line(string buffer_name, int line_num, string content);
  string get_line(string buffer_name, int line_num);
  void print_line(string buffer_name, int line_num);
  void print_lines(string buffer_name, int start_line, int end_line);
};

enum CommandType { BUFFER_CMD, LINE_CMD };

enum BufferCommand { OPEN, PRINT, APPEND, SAVE, NEW };

enum LineCommand {
  REPLACE,
  INSERT,
  DELETE,
  MOVE,
  COPY,
  GET,
  PRINT_LINE,
  PRINT_RANGE
};

struct ParsedCommand {
  CommandType type;
  string buffer_name;

  // For buffer commands
  BufferCommand buffer_cmd;
  string buffer_arg;

  // For line commands
  LineCommand line_cmd;
  int line_number;
  int second_line_number; // For ranged operations
  string line_content;
};

class CommandParser {
public:
  ParsedCommand parse(int argc, char **argv);
  void print_usage();
  bool validate_command(const ParsedCommand &cmd);
};

class BFFEditor {
private:
  BufferManager *buffer_manager;
  CommandParser *parser;

public:
  BFFEditor();
  ~BFFEditor();

  int execute_command(const ParsedCommand &cmd);
  void run(int argc, char **argv);
};

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

BufferManager::BufferManager() {
  current_buffer = nullptr;
  temp_directory = "/tmp/bff_buffers/";

  if (!filesystem::exists(temp_directory))
    filesystem::create_directories(temp_directory);
}

BufferManager::~BufferManager() {
  for (auto &pair : buffers) {
    save_buffer_to_temp(pair.second);
    delete pair.second;
  }
}

Buffer *BufferManager::create_buffer(string name) {
  if (buffers.find(name) != buffers.end())
    return buffers[name]; // Buffer already exists

  Buffer *new_buffer = new Buffer(name);
  buffers[name] = new_buffer;
  return new_buffer;
}

bool BufferManager::select_buffer(string name) {
  auto it = buffers.find(name);
  if (it != buffers.end()) {
    current_buffer = it->second;
    return true;
  }

  load_buffer_from_temp(name);
  it = buffers.find(name);
  if (it != buffers.end()) {
    current_buffer = it->second;
    return true;
  }

  return false;
}

bool BufferManager::open_file(string buffer_name, string file_path) {
  Buffer *buf = create_buffer(buffer_name);

  ifstream file(file_path);
  if (!file.is_open())
    return false;

  buf->lines.clear();
  buf->file_path = file_path;

  string line;
  while (getline(file, line))
    buf->lines.push_back(line);

  file.close();
  buf->is_modified = false;
  save_buffer_to_temp(buf);
  return true;
}

bool BufferManager::replace_line(string buffer_name, int line_num,
                                 string content) {
  Buffer *buf = get_buffer(buffer_name);
  if (!buf || line_num < 1 || line_num > buf->lines.size())
    return false;

  buf->lines[line_num - 1] = content; // line num - 1 due to zero-based indexing
  buf->is_modified = true;
  save_buffer_to_temp(buf);
  return true;
}

bool BufferManager::insert_line(string buffer_name, int line_num,
                                string content) {
  Buffer *buf = get_buffer(buffer_name);
  if (!buf || line_num < 1)
    return false;

  if (line_num > buf->lines.size()) {
    buf->lines.push_back(content);
  } else {
    buf->lines.insert(buf->lines.begin() + line_num - 1, content);
  }

  buf->is_modified = true;
  save_buffer_to_temp(buf);
  return true;
}

bool BufferManager::delete_line(string buffer_name, int line_num) {
  Buffer *buf = get_buffer(buffer_name);
  if (!buf || line_num < 1 || line_num > buf->lines.size())
    return false;

  buf->lines.erase(buf->lines.begin() + line_num - 1);
  buf->is_modified = true;
  save_buffer_to_temp(buf);
  return true;
}

bool BufferManager::move_line(string buffer_name, int from_line, int to_line) {
  Buffer *buf = get_buffer(buffer_name);
  if (!buf || from_line < 1 || to_line < 1 || from_line > buf->lines.size() ||
      to_line > buf->lines.size())
    return false;

  string line_content = buf->lines[from_line - 1];
  buf->lines.erase(buf->lines.begin() + from_line - 1);
  if (to_line > from_line)
    to_line--;

  buf->lines.insert(buf->lines.begin() + to_line - 1, line_content);
  buf->is_modified = true;
  save_buffer_to_temp(buf);
  return true;
}

///////////////////////////////////////////////////////

ParsedCommand CommandParser::parse(int argc, char **argv) {
  ParsedCommand cmd;

  if (argc < 3)
    throw invalid_argument("Insufficient arguments");

  // Parse buffer selection (-b flag)
  if (string(argv[1]) == "-b" && argc > 2) {
    cmd.buffer_name = string(argv[2]);

    if (argc == 3) {
      // On default (with no buffer cmd) - print the buffer
      cmd.type = BUFFER_CMD;
      cmd.buffer_cmd = PRINT;
      return cmd;
    }

    // Parse commands
    string command = string(argv[3]);

    // Checking if buffer command
    if (command == "open" && argc > 4) {
      cmd.type = BUFFER_CMD;
      cmd.buffer_cmd = OPEN;
      cmd.buffer_arg = string(argv[4]);
    } else if (command == "print") {
      cmd.type = BUFFER_CMD;
      cmd.buffer_cmd = PRINT;
    } else if (command == "append" && argc > 4) {
      cmd.type = BUFFER_CMD;
      cmd.buffer_cmd = APPEND;
      cmd.buffer_arg = string(argv[4]);
    } else if (command == "save") {
      cmd.type = BUFFER_CMD;
      cmd.buffer_cmd = SAVE;
      if (argc > 4)
        cmd.buffer_arg = string(argv[4]);
    } else if (command == "new") {
      cmd.type = BUFFER_CMD;
      cmd.buffer_cmd = NEW;
      if (argc > 4)
        cmd.buffer_arg = string(argv[4]);
    }
    // Checking if line command
    else if (command == "line" && argc > 5) {
      cmd.type = LINE_CMD;
      cmd.line_number = stoi(string(argv[4]));

      string line_operation = string(argv[5]);

      if (line_operation == "replace" && argc > 6) {
        cmd.line_cmd = REPLACE;
        cmd.line_content = string(argv[6]);
      } else if (line_operation == "insert" && argc > 6) {
        cmd.line_cmd = INSERT;
        cmd.line_content = string(argv[6]);
      } else if (line_operation == "delete")
        cmd.line_cmd = DELETE;
      else if (line_operation == "move" && argc > 6) {
        cmd.line_cmd = MOVE;
        cmd.second_line_number = stoi(string(argv[6]));
      } else if (line_operation == "copy" && argc > 6) {
        cmd.line_cmd = COPY;
        cmd.second_line_number = stoi(string(argv[6]));
      } else if (line_operation == "get")
        cmd.line_cmd = GET;
      else if (line_operation == "print")
        cmd.line_cmd = PRINT_LINE;
      else if (line_operation == "range" && argc > 6) {
        cmd.line_cmd = PRINT_RANGE;
        cmd.second_line_number = stoi(string(argv[6]));
      }
    }
  }

  return cmd;
}

void CommandParser::print_usage() {
  cout << "bff: bff-technical-preview01" << endl << endl;

  cout << "Usage: bff -b [BUFFER NAME] [BUFFER COMMAND|LINE COMMAND] [COMMAND "
          "ARGUMENT 1] [COMMAND ARGUMENT 2]"
       << endl
       << endl;

  cout << "Usage examples:" << endl << endl;

  cout << "Buffer commands:" << endl;
  cout << "bff -b \"test\" open \"/path/to/file.txt\"" << endl;
  cout << "bff -b \"test\" print" << endl;
  cout << "bff -b \"test\" append \"new content\"" << endl;
  cout << "bff -b \"test\" save \"/new/path/file.txt\"" << endl;
  cout << "bff -b \"test\" new \"/path/to/newfile.txt\"" << endl << endl;

  cout << "Line commands:" << endl;
  cout << "bff -b \"test\" line 10 replace \"return 0;\"" << endl;
  cout << "bff -b \"test\" line 5 insert \"// New comment\"" << endl;
  cout << "bff -b \"test\" line 3 delete" << endl;
  cout << "bff -b \"test\" line 7 move 2" << endl;
  cout << "bff -b \"test\" line 4 copy 8" << endl;
  cout << "bff -b \"test\" line 6 get" << endl;
  cout << "bff -b \"test\" line 2 print" << endl;
  cout << "bff -b \"test\" line 1 range 10" << endl;
}

///////////////////////////////////////////////////////

int BFFEditor::execute_command(const ParsedCommand &cmd) {
  if (cmd.type == BUFFER_CMD) {
    switch (cmd.buffer_cmd) {
    case OPEN:
      if (!buffer_manager->open_file(cmd.buffer_name, cmd.buffer_arg)) {
        cerr << "Error: Could not open file " << cmd.buffer_arg << endl;
        return 1;
      }
      cout << "File opened in buffe '" << cmd.buffer_name << "'" << endl;
      break;
    case APPEND:
      buffer_manager->append_to_buffer(cmd.buffer_name, cmd.buffer_arg);
      cout << "Content appended to buffer '" << cmd.buffer_name << "'" << endl;
      break;
    case SAVE:
      if (!buffer_manager->save_file(cmd.buffer_name, cmd.buffer_arg)) {
        cerr << "Error: Could not dave buffer " << cmd.buffer_name << endl;
        return 1;
      }
      cout << "Buffer '" << cmd.buffer_name << "' saved" << endl;
      break;
    case NEW:
      if (!buffer_manager->create_new_buffer(cmd.buffer_name, cmd.buffer_arg)) {
        cerr << "Error: Coulnd not create new buffer" << endl;
        return 1;
      }
      cout << "New buffer '" << cmd.buffer_name << "' created" << endl;
      break;
    default:
    case PRINT:
      buffer_manager->print_buffer(cmd.buffer_name);
      break;
    }
  } else if (cmd.type == LINE_CMD) {
    switch (cmd.line_cmd) {
    case REPLACE:
      if (!buffer_manager->replace_line(cmd.buffer_name, cmd.line_number,
                                        cmd.line_content)) {
        cerr << "Error: Could not replace line " << cmd.line_number << endl;
        return 1;
      }
      cout << "Line " << cmd.line_number << " replaced in buffer '"
           << cmd.buffer_name << "'" << endl;
      break;
    case INSERT:
      if (!buffer_manager->insert_line(cmd.buffer_name, cmd.line_number,
                                       cmd.line_content)) {
        cerr << "Error: Could not insert line at " << cmd.line_number << endl;
        return 1;
      }
      cout << "Line inserted at position " << cmd.line_number << " in buffer '"
           << cmd.buffer_name << "'" << endl;
      break;
    case DELETE:
      if (!buffer_manager->delete_line(cmd.buffer_name, cmd.line_number)) {
        cerr << "Error: Could not delete line " << cmd.line_number << endl;
        return 1;
      }
      cout << "Line " << cmd.line_number << " deleted from buffer '"
           << cmd.buffer_name << "'" << endl;
      break;
    case MOVE:
      if (!buffer_manager->move_line(cmd.buffer_name, cmd.line_number,
                                     cmd.second_line_number)) {
        cerr << "Error: Could not move line" << endl;
        return 1;
      }
      cout << "Line " << cmd.line_number << " moved to position "
           << cmd.second_line_number << endl;
      break;
    case GET:
      cout << buffer_manager->get_line(cmd.buffer_name, cmd.line_number)
           << endl;
      break;
    case PRINT_LINE:
      buffer_manager->print_line(cmd.buffer_name, cmd.line_number);
      break;
    case PRINT_RANGE:
      buffer_manager->print_lines(cmd.buffer_name, cmd.line_number,
                                  cmd.second_line_number);
      break;
    }
  }
  return 0;
}

void BFFEditor::run(int argc, char **argv) {
  try {
    ParsedCommand cmd = parser->parse(argc, argv);

    if (!parser->validate_command(cmd)) {
      parser->print_usage();
      return;
    }

    int result = execute_command(cmd);
    exit(result);
  } catch (const exception &e) {
    cerr << "Error: " << e.what() << endl;
    parser->print_usage();
    exit(1);
  }
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

int main(int argc, char **argv) {
  BFFEditor editor;
  editor.run(argc, argv);
  return 0;
}
