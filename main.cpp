#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories);

bool ChekInclude(const path& in_file, const path& out_file, const string& str, const vector<path>& include_directories, int n) {

    ofstream out(out_file, ios::out | ios::app | ios::binary);

    static regex include_up(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    smatch inc_up;
    static regex include_oper(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    smatch inc_oper;

    path p;

    if (regex_match(str, inc_up, include_up)) {
        path x = string(inc_up[1]);
        p = in_file.parent_path() / x;
        ifstream chek_include_up(p);
        if (chek_include_up) {
            Preprocess(p, out_file, include_directories);
            return true;
        }
        else {
            for (const auto& include : include_directories) {
                ifstream stream(include / x, ios::binary);
                if (!stream) {
                    continue;
                }
                else {
                    stream.close();
                    p = include / x;
                    break;
                }
            }
            ifstream chek_include_oper(p);
            if (!chek_include_oper) {
                cout << "unknown include file "s << p.filename().string() << " at file "s << in_file.string() << " at line "s << n << endl;
                return false;
            }
            else {
                Preprocess(p, out_file, include_directories);
                return true;
            }
        }
    }

    if (regex_match(str, inc_oper, include_oper)) {
        for (const auto& include : include_directories) {
            path x = string(inc_oper[1]);
            p = include / x;
            ifstream stream(p, ios::binary);
            if (!stream) {
                continue;
            }
            else {
                stream.close();
                p = include / x;
                break;
            }
        }
        ifstream chek_include_oper(p);
        if (!chek_include_oper) {
            cout << "unknown include file "s << p.filename().string() << " at file "s << in_file.string() << " at line "s << n << endl;
            return false;
        }
        else {
            Preprocess(p, out_file, include_directories);
            return true;
        }
    }

    out << str << endl;
    out.close();
    return true;
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {

    ifstream stream(in_file, ios::binary);
    if (!stream) {
        return false;
    }
    int n = 0;
    string str = "";
    while (getline(stream, str)) {
            ++n;
        if (!ChekInclude(in_file, out_file, str, include_directories, n)) {
            return false;
            }
    }
    return true;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return { (istreambuf_iterator<char>(stream)), istreambuf_iterator<char>() };
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
            "#include \"dir1/b.h\"\n"
            "// text between b.h and c.h\n"
            "#include \"dir1/d.h\"\n"
            "\n"
            "int SayHello() {\n"
            "    cout << \"hello, world!\" << endl;\n"
            "#   include<dummy.txt>\n"
            "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
            "#include \"subdir/c.h\"\n"
            "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
            "#include <std1.h>\n"
            "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
            "#include \"lib/std2.h\"\n"
            "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.text"_p,
        { "sources"_p / "include1"_p,"sources"_p / "include2"_p })));

    ostringstream test_out;
    test_out << "// this comment before include\n"
        "// text from b.h before include\n"
        "// text from c.h before include\n"
        "// std1\n"
        "// text from c.h after include\n"
        "// text from b.h after include\n"
        "// text between b.h and c.h\n"
        "// text from d.h before include\n"
        "// std2\n"
        "// text from d.h after include\n"
        "\n"
        "int SayHello() {\n"
        "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
