#pragma once
#include "rst.h"

namespace html {

// An rst file converted into HTML.
//
class post
{
    public:
    // The name (derived from the file name) for this post.
    std::string name;
    // Day of post in YYYY-MM-DD format
    std::string date;
    std::string title;
    // The html content of the post
    std::stringstream data;    
    // A copy of the first paragraph of the post.
    std::stringstream first_para; 
};

static inline std::string html(const std::string &s)
{
    std::string ret;
    for (auto &c : s)
    {
        if (c == '\"')
            ret += "&quot;";
        else if (c == '&')
            ret += "&amp;";
        else if (c == '\'')
            ret += "&apos;";
        else if (c == '<')
            ret += "&lt;";
        else if (c == '>')
            ret += "&gt;";
        else
            ret += c;
    }
    return ret;
}

// Create a post from a parsing of an rst file.
//
class output : public rst::outputter
{
    public:
        
    post &dest;
        
    output(post &_dest) : dest(_dest) {}
    
    // styled_line -> html translation
    void style_line(std::ostream &os, const rst::styled_line &l)
    {
        for(auto &c : l)
        {
            switch(c.style)
            {
                case rst::Normal:
                    os << html(c.data);
                    break;
                case rst::Starred:
                    os << "<b>" << html(c.data) << "</b>";
                    break;
                case rst::Code:
                    os << "<code>" << c.data << "</pre></code>";
                    break;
                case rst::Link:
                    os << "<a href=\"" << c.extra << "\">" << html(c.data) << "</a>";
                    break;
                default:
                    break;
            }
        }
    }

    virtual void normal(const rst::styled_paragraph &paragraph)
    {
        for(auto &l : paragraph)
        {
            dest.data << "<p>";
            style_line(dest.data, l);
            dest.data << "</p>\n";
            
            if (dest.first_para.str().empty())
            {
                dest.first_para << "<p>";
                style_line(dest.first_para, l);
                dest.first_para << "</p>\n";
            }
        }
    }   

    virtual void bulletlist(const rst::styled_paragraph &paragraph) 
    {
        dest.data << "<ul>\n";
        for(auto &l : paragraph)
        {
            dest.data << "<li><p>";
            style_line(dest.data, l);
            dest.data << "</p></li>\n";
        }
        dest.data << "</ul>\n";
    }
    
    virtual void table(const rst::styled_paragraph &paragraph, 
        const std::vector<int> &columns, std::vector<int> &rows) 
    {
        // TODO:
    }
    
    virtual void directive(const std::string &name, const std::string &data, 
        const std::map<std::string, std::string> &ctags, 
        const std::vector<std::string> &paragraph)
    {
        // Directives contain meta data and/or formatting
        // commands.
        //
        std::map<std::string, std::string> tags = ctags;
        if (name == "post")
        {
            // Meta data for this post.
            dest.title = html(tags["title"]);
            dest.date = data;
        }   
        else if (name == "math")
        {
            for(auto &l : paragraph)
            {
                dest.data << "$$\n";
                dest.data << l << "\n";
                dest.data << "$$\n";
            }
        }
        else if (name == "image")
        {
            dest.data << "<img src=\"" << data << "\"><br/>\n";
            dest.data << "<br/>\n";
        }
    }
    
    virtual void code(const std::vector<std::string> &paragraph)
    {
        dest.data << "<code><pre>\n";
        for(auto &l : paragraph)
            dest.data << l << "\n";
        dest.data << "</pre></code>\n";
    }
        
    virtual void header(const rst::styled_line &str) 
    {
        dest.data << "<h2>";
        style_line(dest.data, str);
        dest.data << "</h2>\n";
    }
    
    virtual void subheader(const rst::styled_line &str)
    {
        dest.data << "<h3>";
        style_line(dest.data, str);
        dest.data << "</h3>\n";
    }
};

// Simple, not-necessarily html, template instantiation.
//
// Copy a file to \p out, replacing all instances of 
// <!-- INSERT <word> --> lines into calls to \p handler(<word>).
//
void process_template(const std::string &tname, std::ostream &out, 
    std::function<void(const std::string &, std::ostream &)> handler)
{
    std::ifstream fin(tname);
    
    std::regex rgx(".*<!-- INSERT ([a-zA-Z]+) -->.*");
    
    while(fin)
    {
        std::string line;
        std::getline(fin, line);

        std::smatch matches;
        if (std::regex_search(line, matches, rgx) && (matches.size() == 2)) 
            handler(matches[1], out);
        else
            out << line << "\n";
    }
}

}