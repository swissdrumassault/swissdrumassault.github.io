#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <regex>

namespace rst {

// A description of a paragraph of marked-up text, being a vector 
// of styled lines, each containing a vector of styled strings.

// How should this string is interpreted?
enum style_t : int
{
    // Things with corresponding HTML tags 
    Normal, Escaped, Starred, Code, Link, Maths,
    // Currently unused
    Bracketed, 
    // Not-really styles, used during parsing
    Invalid, MaybeParen
};

// A string with a style and (optionally) some extra data.
//
class styled_string
{
    public:
        style_t style = Normal;
        std::string data;
        std::string extra;
        
    styled_string() {}
    styled_string(const std::string &s) : data(s) {}
    styled_string &operator =(const std::string &s) { data = s; return *this; }
};

// An ordered set of styled strings
//
class styled_line
{
    public:
        typedef std::vector<styled_string> data_t;
        typedef data_t::iterator iterator;
        typedef data_t::const_iterator const_iterator;
        
        data_t data;
        
        const_iterator begin() const { return data.begin(); }
        const_iterator end() const { return data.end(); }
        iterator begin() { return data.begin(); }
        iterator end() { return data.end(); }
        
    void push_back(const styled_string &s) { data.push_back(s); }
    
    styled_line() {}
    styled_line(const std::string &s) { data.assign(1, s); }
    styled_line &operator =(const std::string &s) { data.assign(1, s); return *this; }
};

// An ordered set of styled strings
//
typedef std::vector<styled_line> styled_paragraph;

// Interface of the thing that will recieve callbacks when various
// formatting constructs are seen in the input file.
//
class outputter
{
    public:

    // A normal block of text.
    virtual void normal(const styled_paragraph &paragraph) = 0;
    
    // A block of text where each line is a bullet point.
    virtual void bulletlist(const styled_paragraph &paragraph) = 0;
    
    // A table.
    virtual void table(const styled_paragraph &paragraph, 
        const std::vector<int> &columns, std::vector<int> &rows) = 0;
        
    virtual void directive(const std::string &name, const std::string &data, 
        const std::map<std::string, std::string> &tags, 
        const std::vector<std::string> &paragraph) = 0;
        
    virtual void code(const std::vector<std::string> &paragraph) = 0;
    virtual void header(const styled_line &str) = 0;
    virtual void subheader(const styled_line &str) = 0;
};

// Debug descriptions of styled strings and lines.
//
static inline std::ostream &operator <<(std::ostream &os, const styled_string &s)
{
    if (s.style != Normal)
        os << s.style << "(" << s.data << ")";
    else
        os << s.data;
    if (!s.extra.empty())
        os << "EXTRA(" << s.extra << ")";
    return os;
}

static inline std::ostream &operator <<(std::ostream &os, const styled_line &l)
{
    for(auto &s : l.data)
        os << s;
    return os;
}

// A handler that emits debug descriptions of things.
//
class debug_output : public outputter
{
    public:
    
    void normal(const std::vector<styled_line> &paragraph) override 
    { 
        for(auto &l : paragraph)
            std::cerr << "NORMAL: " << l << "\n";
        std::cerr << "BREAK:\n";
    }   

    void bulletlist(const std::vector<styled_line> &paragraph) override 
    { 
        for(auto &l : paragraph)
            std::cerr << "BULLET: " << l << "\n";
        std::cerr << "BREAK:\n";
    }   

    void table(const std::vector<styled_line> &paragraph, 
        const std::vector<int> &columns, std::vector<int> &rows)override 
    {
        std::cerr << "ROWS: ";
        for(auto &r : rows)
            std::cerr << r << ", ";
        std::cerr << "\n";
        std::cerr << "COLS: ";
        for(auto &c : columns)
            std::cerr << c << ",";
        std::cerr << "\n";
                
        for(auto &l : paragraph)
            std::cerr << "TABLE: " << l << "\n";
        std::cerr << "BREAK:\n";
    }  

    void code(const std::vector<std::string> &paragraph) override 
    {
        for(auto &l : paragraph)
            std::cerr <<  "CODE : " << l << "\n";
        std::cerr << "BREAK:\n";
    } 
    
    void directive(const std::string &name, const std::string &data, 
        const std::map<std::string, std::string> &tags, 
        const std::vector<std::string> &paragraph) override
    {
        std::cerr << "DSTART " << name << " : " << data << "\n";
        for(auto &kv : tags)
            std::cerr << "DKEY " << kv.first << "=" << kv.second << "\n";
        for(auto &l : paragraph)
            std::cerr <<  "DIRECTIVE : " << l << "\n";
    } 
        
    void header(const styled_line &str) override 
    {
        std::cerr <<  "HEADER : " << str << "\n";
    } 

    void subheader(const styled_line &str) override 
    {
        std::cerr <<  "SUBHEADER : " << str << "\n";
    } 
};

// Utility functions
//
static inline bool all_certain_chars(const std::string &s, const std::string &domain)
{
    return s.find_first_not_of(domain) == std::string::npos;
}

static inline size_t utf8len(const std::string &s)
{   
    // FIXME: Deal with multichars properly
    return s.size();
}

// Remove space from beginning and end of string.
static inline std::string strip(const std::string &f)
{
    size_t lpos = f.find_last_not_of(" \n\r\t");
    size_t bpos = f.find_first_not_of(" \n\r\t");
    
    if (lpos == std::string::npos)
        return "";
    else
        return f.substr(bpos, lpos+1-bpos);
}

// Part of the style() algorithm. When leaving a state, generate
// a styled string of the appropriate type and clear the character 
// variables.
//
static inline void flush(std::string &current, styled_line &ret, style_t style, const std::string &extra = "")
{
    if (!current.empty())
    {
        styled_string l;
        l.data = current;
        l.style = style;
        l.extra = extra;
        ret.push_back(l);
    }
    current.clear();
}

// Turn a string into a styled_string.
// Parse out emphasis, images, links, etc.
//
static inline styled_line style(const std::string &l)
{
    // The thing that will be returned - destination for things
    // generated by the state machine.
    styled_line ret;
   
    // Current state of the state machine.
    style_t state = Normal;
    // Characters collected by current state
    std::string current;
    // Extra data collected by current state.
    std::string extra;
    
    // The state to return to after leaving state:Escaped
    style_t pop = Invalid;

    // Feed characters to the state machine
    //
    for(auto &c : l)
    {
        switch(state)
        {
            default:
                throw std::runtime_error("Bad state");
                break;
            case Normal:
                // Detect the beginning of style specifications.
                if (c == '*')
                {
                    flush(current, ret, state);
                    state = Starred;
                }
                else if (c == '`')
                {
                    flush(current, ret, state);
                    state = Code;                    
                }
                else if (c == '$')
                {
                    flush(current, ret, state);
                    state = Maths;                    
                }
                // Detect meta-data inserted into the text
                else if (c == '[')
                {
                    flush(current, ret, state);
                    state = Bracketed;
                }
                // Esacaped characters start with a slash
                else if (c == '\\')
                {
                    pop = state;
                    state = Escaped;
                }
                // Otherwise, cache the character and stay in this state.
                else
                    current += c;
                break;
            case Starred:
                // Look for end
                if (c == '*')
                {
                    flush(current, ret, state);
                    state = Normal;
                }
                // and any escaped characters.
                else if (c == '\\')
                {
                    pop = state;
                    state = Escaped;
                }
                else
                    current += c;
                break;    
            case Maths:
                if (c == '$')
                {
                    flush(current, ret, state);
                    state = Normal;
                }
                // NB: Math mode (i.e. latex) heavily uses slashes
                // so don't consider them escape characters here.
                else
                    current += c;
                break;        
            case Code:
                if (c == '`')
                {
                    flush(current, ret, state);
                    state = Normal;
                }
                else if (c == '\\')
                {
                    pop = state;
                    state = Escaped;
                }
                else
                    current += c;
                break;            
            case Escaped:
                // All characters are cached,
                current += c;
                // but this state is exited after only 1.
                state = pop;
                pop = Invalid;
                break;
            case Bracketed:
                if (c == ']')
                {
                    // NB: Don't flush here in case there is a ()
                    // clause afterward
                    state = MaybeParen;
                }
                else if (c == '\\')
                {
                    pop = state;
                    state = Escaped;
                }
                else
                    current += c;
                break;
            case MaybeParen:
                // A bracketed phrase may be followed by a set
                // of extra data enclosed in ()                
                if (c == '(')
                {
                    // There is some extra data - this is a link.
                    state = Link;
                    extra.clear();
                }
                else
                {
                    // No extra data, emit a Bracketed string.
                    flush(current, ret, Bracketed);
                    current += c;
                    state = Normal;
                }
                break;
            case Link:
                // Process extra data after []
                if (c == ')')
                {
                    flush(current, ret, state, extra);
                    state = Normal;
                }
                else if (c == '\\')
                {
                    pop = state;
                    state = Escaped;
                }
                else
                    extra += c;
                break;
        }
    }
    
    flush(current, ret, state);
    
    return ret;
}

// Turn a vector of strings into a styled paragraph.
//
static inline styled_paragraph style(const std::vector<std::string> &paragraph)
{
    styled_paragraph ret;
    
    for(auto &l : paragraph)
        ret.push_back(style(l));
    return ret;
}

// Work out what type this paragraph is: normal, table or bullet list, and call outputter
// appropriatly.
//
static inline void choose_table_or_paragraph(const std::vector<std::string> &paragraph, outputter &out)
{
    if (paragraph.empty()) return;
    
    bool valid = true;
    size_t width = 0;
    std::vector<int> columns;
    std::vector<int> rows;
    
    if ((paragraph.size() >= 3) && (paragraph[0] == paragraph[paragraph.size() - 1]) &&
        !paragraph[0].empty() && (paragraph[0][0] == '+'))
    {   
        int rheight = -1;
        for(size_t i = 0; i < paragraph.size(); ++i)
        {
            const std::string line = strip(paragraph[i]);
            
            if (line.empty())
            {
                valid = false;
                break;
            }
            
            if (!i)
                width = line.size();
            
            if (line.size() != width)
            {
                valid = false;
                break;
            }

            if (line[0] == '+')
            {
                std::vector<int> tcolumns;
                size_t pos = 0;
                for(;;)
                {
                    size_t npos = line.find('+', pos + 1);
                    if (npos == std::string::npos)
                        break;
                    tcolumns.push_back(npos - pos);
                    pos = npos;
                }
                //std::cerr << "Line " << line << " has " << tcolumns.size() << " columns\n";
                
                if (columns.empty())
                   columns = tcolumns;
                else if (columns != tcolumns)
                {
                    valid = false;
                    break;
                }
                
                if (i)
                    rows.push_back(rheight);
                rheight = 0;
            }
            else if (line[0] == '|')
            {
                size_t next = 0;
                for(size_t j = 0; j < columns.size(); ++j)
                {
                    next += columns[j];
                    if (line[next] != '|')
                    {
                        valid = false;
                        break;
                    }
                }
                //std::cerr << "Line " << line << " is valid \n";
                rheight++;
            }
            else
            {
                valid = false;
                break;
            }
        }
    }
    
    if (valid && !rows.empty() && !columns.empty())
        out.table(style(paragraph), columns, rows);
    else
    {
        bool bulletlist = true;
        for(auto &s : paragraph)
            if (s.find("- ") != 0)
                bulletlist = false;
        
        if (bulletlist)
        {
            // Trim bullet markers from paragraph
            auto tparagraph = paragraph;
            for(auto &s : tparagraph)
                s = s.substr(2);
            out.bulletlist(style(tparagraph));
        }
        else
            out.normal(style(paragraph));
    }
}

// Is this line the start of a directive?
// Return name and any extra data in \p name and \p data.
//
static inline bool directive_start(const std::string &line, std::string &name, std::string &data)
{
    if (line.empty()) 
        return false;
 
    std::regex rgx("\\.\\. ([a-zA-Z]+):: ?(.*)?");
    std::smatch matches;
    
    if (!std::regex_search(line, matches, rgx)) 
        return false;

    if (matches.size() > 1)
        name = matches[1];
    else
        name = "";
        
    if (matches.size() > 2)
        data = matches[2];
    else
        data = "";

    return true;
}

// Is this line a directive tag?
// return key and value, if it is.
//
static inline bool directive_tag(const std::string &line, std::string &key, std::string &value)
{
    if (line.empty()) 
        return false;
    
    std::regex rgx(" *:([a-zA-Z]+): ?(.*)?");
    std::smatch matches;
    
    if (!std::regex_search(line, matches, rgx)) 
        return false;

    if (matches.size() > 1)
        key = matches[1];
    else
        key = "";
        
    if (matches.size() > 2)
        value = matches[2];
    else
        value = "";

    return true;
}

// The main entry point: Turn a set of lines into calls to an outputter.
//
static inline void parse(const std::vector<std::string> &lines, outputter &out)
{
    std::vector<std::string> paragraph;
    std::string dname, ddata;
    
    for(size_t i = 0; i < lines.size(); ++i)
    {
        const std::string &line = lines[i];
        //std::cerr << "LINE: " << line.size() << " " << line << "\n";
        
        if (line.empty() || all_certain_chars(line, " "))
        {
            // paragraph break
            if ((paragraph.size() == 2) && (utf8len(paragraph[1]) == utf8len(paragraph[0])))
            {
                if (all_certain_chars(paragraph[1], "="))
                    out.header(paragraph[0]);
                else if (all_certain_chars(paragraph[1], "-"))
                    out.subheader(paragraph[0]);
                else
                    choose_table_or_paragraph(paragraph, out);
            }
            else if ((paragraph.size() == 1) && (strip(paragraph[0]) == "::"))
            {
                // beginning of code block
                ++i;
                
                paragraph.clear();
                while((i < lines.size()) && (lines[i].empty() || (lines[i][0] == ' '))) 
                {
                    paragraph.push_back(lines[i]);
                    ++i;
                }
                --i;
                
                out.code(paragraph);
            }
            else if ((!paragraph.empty()) && directive_start(paragraph[0], dname, ddata))
            {
                // beginning of directive block
                ++i;
                
                while((i < lines.size()) && (lines[i].empty() || (lines[i][0] == ' '))) 
                {
                    paragraph.push_back(lines[i]);
                    ++i;
                }
                --i;

                // Remove tag specification lines from paragraph.
                std::vector<std::string> nparagraph;
                paragraph.erase(paragraph.begin());
                
                std::map<std::string, std::string> tags;
                for(auto &line : paragraph)
                {
                    std::string k, v;
                    if (directive_tag(line, k, v))
                        tags[k] = v;
                    else if (strip(line) != "")
                        nparagraph.push_back(line);
                }
                
                out.directive(dname, ddata, tags, nparagraph);
            }
            else
                choose_table_or_paragraph(paragraph, out);
            
            paragraph.clear();
        }
        else
            paragraph.push_back(line);
    }
}

}