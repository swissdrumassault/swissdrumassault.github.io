#pragma once
#include <set>
#include "rst.h"
#include "porterstemmer.h"

// /usr/lib64/python3.4/site-packages/sphinx/search/__init__.py
// /usr/lib64/python3.4/site-packages/sphinx/search/en.py
// https://bitbucket.org/methane/porterstemmer/src/cbc0d6793783d07bc89ac1cd1d23f701d20ec54a/porterstemmer.c?at=default&fileviewer=file-view-default

namespace search
{

class index
{
    public:
        std::map<std::string, std::set<std::string> > words;
        std::map<std::string, std::string> titles;
};

void dump(const index &src, std::ostream &out)
{
    out << "Search.setIndex(\n";
    out << "{envversion:46,\n";
    
    std::map<std::string, std::vector<int> > pivot;
    int n = 0;
    for(auto &kv : src.words)
    {
        for(auto &w : kv.second)
            pivot[w].push_back(n);
        ++n;
    }
    
    out << "filenames:[";
    bool first = true;
    for(auto &kv : src.words)
    {   
        if (!first)
            out << ",";
        out << "\"" << kv.first << "\"";
        first = false;
    }
    out << "],\n";
    
    out << "objects:{},objnames:{},objtypes:{},\n";
    out << "terms:{";
    first = true;
    for(auto &kv : pivot)
    {
        if (!first)
            out << ",";
        out << "\"" << kv.first << "\":";
        first = false;
        
        if (kv.second.size() > 1)
        {
            out << "[";
            bool lfirst = true;
            for(auto &i : kv.second)
            {
                if (!lfirst)
                    out << ",";
                out << i;
                lfirst = false;
            }
            out << "]";
        }
        else
            out << kv.second[0];
    }
    out << "},\n";
    
    out << "titles:[";
    first = true;
    for(auto &kv : src.words)
    {   
        if (!first)
            out << ",";
        auto f = src.titles.find(kv.first);
        if (f != src.titles.end())
            out << "\"" << (*f).second << "\"";
        else
            out << "\"" << kv.first << "\"";
        
        first = false;
    }
    out << "], titleterms:[]\n";
    
    out << "})\n";
    

}

class output : public rst::outputter
{
    public:
        
    index &dest;
    
    std::string fname;
    
    std::set<std::string> stopwords{
        "a", "and", "are", "as", "at",  "be", "but", "by", "for", "if", "in", "into", 
        "is", "it",
        "near", "no", "not", "of", "on", "or", "such",
        "that", "the", "their", "then", "there", "these", "they", "this", "to",
        "was",  "will", "with"
        };
    
    output(const std::string &_fname, const std::string &title, index &_dest) 
        : fname(_fname), dest(_dest) 
    {
        dest.titles[fname] = title;
    }
    
    void add_word(std::string w)
    {
        // Use porterstemmer to find word's stem.
        int k = stem(&w[0], w.size() - 1);
        w = w.substr(0, k+1);

        // do word_filter() from /usr/lib64/python3.4/site-packages/sphinx/search/__init__.py
        //
        // len(word) == 0 or not 
        // (
        //        ((12353 < ord(word[0]) < 12436) and (len(word) < 3)) 
        //    or
        //        (ord(word[0]) < 256 and (len(word) < 3 or word in self.stopwords or word.isdigit()))
        // )
        
        if (w.empty()) return;
        if (w.size() < 3) return;
        if (stopwords.find(w) != stopwords.end()) return;
        
        // record word in index.
        dest.words[fname].insert(w);
    }
    
    // Find words in string
    //
    void add_words(const std::string &s)
    {
        std::string c = rst::strip(s);
        
        // Strip out non-alpha-numeric characters.
        for(auto &cc : c)
            if (isalnum(cc))
                cc = tolower(cc);
            else
                cc = ' ';
                
        // Split on space
        while(!c.empty())
        {
            size_t space = c.find_first_of(" \t\r\n");
            
            if ((space == std::string::npos) && (!c.empty()))
            {
                add_word(c);
                break;
            }
            else
            {
                add_word(c.substr(0, space));
                c = rst::strip(c.substr(space + 1));
            }
        }
    }
    
    void add_words(const rst::styled_line &l)
    {
        for(auto &c : l)
            add_words(c.data);
    }

    virtual void normal(const rst::styled_paragraph &paragraph)
    {
        for(auto &l : paragraph)
            add_words(l);
    }   

    virtual void bulletlist(const rst::styled_paragraph &paragraph) 
    {
        for(auto &l : paragraph)
            add_words(l);
    }
    
    virtual void table(const rst::styled_paragraph &paragraph, 
        const std::vector<int> &columns, std::vector<int> &rows) 
    {
        for(auto &l : paragraph)
            add_words(l);
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
            add_words(tags["title"]);
        }   
    }
    
    virtual void code(const std::vector<std::string> &paragraph)
    {
        for(auto &l : paragraph)
            add_words(l);
    }
        
    virtual void header(const rst::styled_line &str) 
    {
        add_words(str);
    }
    
    virtual void subheader(const rst::styled_line &str)
    {
        add_words(str);
    }
};



};