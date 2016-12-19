#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <regex>

#include "rst.h"
#include "rst_html.h"
#include "rst_search.h"

std::string remove_extension(const std::string &fname)
{
    size_t dot = fname.rfind('.');
    if (dot == std::string::npos)
        return fname;
    else
        return fname.substr(0, dot);
}

int main(int argc, char *argv[])
{
    // The rst files converted into html.
    std::vector<html::post> posts(argc - 1);

    // The search index over all posts.
    search::index index;
    
    // Iterate over inputs.
    for(int i = 1; i < argc; ++i)
    {
        // Load file as collection of lines.
        std::vector<std::string> lines;
        
        std::ifstream fin(argv[i]);
        while (fin)
        {
            std::string line;
            std::getline(fin, line);
            if (!fin) break;
            lines.push_back(line);
        }
        lines.push_back("");
        
        std::cerr << "Read " << lines.size() << " lines from " << argv[i] << "\n";
    
        //debug_output out;
        
        // Convert to html.
        html::post &post = posts[i-1];
        html::output out(post);
        post.name = remove_extension(argv[i]);        
        rst::parse(lines, out);
        
        // Collect search terms into index.
        search::output sout(post.name, post.title, index);
        rst::parse(lines, sout);
    }
    
    std::sort(posts.begin(), posts.end(), 
        [](const html::post &a, const html::post &b) { return a.date > b.date; });
        
    // List of posts for naviagation bar
    std::stringstream postlist;
    postlist << "<ul>\n";
    postlist << "<li><a href=\"index.html\">top</a></li>\n";
    postlist << "</ul>\n";
    postlist << "<br/>\n";
    postlist << "<ul>\n";  
    for(auto &p : posts)
    {
        postlist << "<li><a href=\"" << p.name << ".html\">" << p.date << " - " << 
            p.title << "</a></li>\n";
    }
    postlist << "</ul>\n";

    // Content of main page
    std::stringstream recents;
    recents << "<ul>\n";
    recents << "<h1>Some descriptions of some of my projects</h1>\n";
    recents << "<p>Here is a list of most recent posts:</p>\n";
    recents << "<ul class=\"postlist-style-none postlist\">\n";
    
    for(auto &p : posts)
    {
        recents << "<li><p class=\"first\">" << p.date << " - " << 
            "<a href=\"" << p.name << ".html\">" << p.title << "</a></p>\n";
            
        recents << p.first_para.str();
    }
    recents << "</ul>\n";

    // Make html files for posts by inserting html into template.
    //
    for(auto &p : posts)
    {
        std::ofstream fout((p.name + ".html").c_str());
        
        html::process_template("template.html", fout, 
            [&p, &postlist](const std::string &tag, std::ostream &os)
            {
                if (tag == "title")
                    os << "<title>" << p.title << "</title>";
                else if (tag == "content")
                    os << p.data.str();
                else if (tag == "navigation")
                {
                    os << "<h2>" << p.date << "</h2>\n";
                    os << postlist.str();
                }
            });
    }
    
    // Make main page
    //
    {
        std::ofstream fout("index.html");
        html::process_template("template.html", fout, 
            [&postlist, &recents](const std::string &tag, std::ostream &os)
            {
                if (tag == "title")
                    os << "<title> Some Descriptions Of Some Of My Projects</title>";
                else if (tag == "content")
                {
                    os << recents.str();
                }
                else if (tag == "navigation")
                {
                    os << postlist.str();
                }
            });
    }
    
    // Write search index.
    {
        std::ofstream fout("searchindex.js");
        search::dump(index, fout);
    }
}