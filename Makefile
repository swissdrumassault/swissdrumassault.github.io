CXXFLAGS += -std=c++11 -O3

PROGS = blog website

all: $(PROGS)

clean:
	-rm *.o $(PROGS)

blog.o: blog.cpp rst.h rst_html.h rst_search.h porterstemmer.h 
	$(CXX) $(CXXFLAGS) -c $(filter-out %.h,$^) -o $@ 

blog: blog.o porterstemmer.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)


POSTS= \
	post-2016-02-02.rst \
	post-2016-02-03.rst \
	post-2016-02-03a.rst \
	post-2016-02-05.rst \
	post-2016-02-14.rst \
	post-2016-03-18.rst \
	post-2016-06-12.rst \
	post-2016-12-19.rst 
	
	
website: $(POSTS) template.html
	./blog $(filter %.rst,$^)
	
