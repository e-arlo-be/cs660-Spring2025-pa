# PA0 Writeup

## Approach
Disclaimer: I am a C programmer at heart but very new to C++ so this was a learning experience that involved a lot of trial and error, so there are probably many cases where in an attempt to get things working I did some wonky things since I am unfamiliar with the syntax and the standard library functions. 

I worked alone on this assignment. I spent the first day getting my environment set up and ensuring I could build the given source files and familiarizing myself with the existing code and instructions to make sure I understood how things should hook together. After that I spent a full day implementing the todos in the code, and a few hours debugging by reading the test files and ensuring my code adhered to the expected interfaces. 

### Database

I chose to use an unordered map for the catalog so I could uniquely identify DbFiles by their file names, so lookup, retrieval, and deletion of DbFiles are more efficient.  

#### Database::add

I check the catalog to see if the file already exists there. I had intended to use "contains" to check this more efficiently, but ran into errors so I ended up using find and iterating over the catalog which is definitely not the most efficient way but it works. If the file is found an error is thrown, otherwise I add the file to the catalog, using move to transfer ownership. 

#### Database::remove

Similar to add, I use find to iterate over my catalog to sheck if the file extsts, and if it does not an error is thrown. If the file is found, flushFile is called to flush the data to the disk if it contains any pages marked dirty. I use move to return the file while transferring ownership to the caller and erase the entry in the catalog. 

#### Database::get

Check that the name exists in the catalog, throw an error if not. If it exists, return a pointer to the element in the catalog corresponding to the file name. 

### BufferPool

I added 4 private members to the BufferPool Class: An unordered map to map Pages based on their PageId, an unordered set of PageIds to track which are dirty, and a vector of PageIds that I use as a FIFO queue to track LRU ordering, as well as an integer to track the number of pages in use in the buffer at any given time. 

#### BufferPool constructors and destructors

I am not sure if I handled these completely correctly, but my constructor simply initializes the number of pages to 0. The destructor uses the dirty vector to flush all outstanding dirty pages to disk before clearing the pages, dirty, and lru data structures.

#### BufferPool::getPage

First a helper function updates the lru information to reflect that the requested page is the most recently used. If the page exists in the pages map, a reference to the page is returned. Otherwise the number of pages is checked to determine if eviction is necessary. If the maximin number of pages are used, the first element in the LRU queue is flushed and evicted. The new page is read into the pages map from disk and returned. 

#### BufferPool::markDirty
If the pageId is not already in the dirty set, it is added.

#### BufferPool::isDirty
First checks that the page exists in the BufferPool, if so the dirty set is consulted and if the PageId is present returns true, false otherwise.

#### BufferPool::contains
Searches the pages map, returns true if an entry matching the PageId is found.

#### BufferPool::discardPage
Checks if the page is contained in the pages map, if not there is nothing to do so return immediately. If the page is present, it is deleted from pages, and number of pages is decremented before removing the PageID from the dirty set and lru queue. 

#### BufferPool::flushPage
If the page is present in the pages map and is also marked dirty in the dirty set, the buffered page is writen to the disk and its PageId is removed from the dirty set.

#### BufferPool::flushFile
Iterates over the dirty set, maintaining a list of the PageIds whose names match the named file, then calls flushPage on all matching PageIds. 

#### BufferPool::updateLRU
Consults the LRU queue, if the PageId already exists, it is deleted and added to the end of the queue, otherwise it is just added to the end. 

### Questions

1. When a page is marked dirty, it can also be moved to the back of the LRU queue to increase the chance that the page at the front of the queue chosen for eviction is a clean page.

2. Leaving the pages in the buffer pool when the file has been removed could lead to the pages being successfully accessed through the buffer pool even though they no longer exist in the Database. 

3. In a situation like Question 2 where the file has been removed from the Database, one would want to be able to discard the page from the buffer pool without writing it to the disk.


