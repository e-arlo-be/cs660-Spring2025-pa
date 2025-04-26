# Implementation

## Projection

I first use the tuple descriptor from the input file to create a vector of the 
indices corresponding to the fields passed in. I iterate over all tuples in 
the input file. For each input tuple, I create a vector of the fields that will 
be projected using the indices vector, and create a new tuple with only these 
fields that is added to the output file. 

## Filter

### filter-tuples helper function

I decided to implement this as a helper function because I assumed it could be 
useful to have factored out for re-use in other parts of the code but I 
realize now its not necessary but regardless:

For each tuple in the input file, I loop over all predicates in the predicate 
vector and use a switch statement based on the predicate op to determine if 
the tuple passes. On the first failure, the inner loop breaks (since we do not
need to check remaining predicates if one fails) and moves onto the next tuple. 
If all predicates pass for a given tuple, a boolean called match will be true 
when the inner loop completes, causing the tuple to be added to the vector of 
tuples that this function returns. 

### filter
Filter calls the filter-tuples helper function, and inserts all of the tuples 
from the returned vector into the output file. 

## Aggregate

### aggregate-tuples helper function

In order to simplify programming the group by behavior, I wrote a helper 
that handles calculating the aggregates of a single vector of tuples, and 
returns a single tuple with one field containing the aggregate value. Since the 
field type is a variant, this function performs the appropriate checks to 
determine whether the data type is an int or double, and handles each properly. 

### aggregate

In the case where there is no group by, aggregate simply creates a vector of 
all tuples in the input file and hands it over to the helper function, then 
inserts the returned tuple into the output file. 

When there is a group by, I create a hash table based on the values of the 
group by fields, mapping each unique value of the field to a vector of the 
tuples that have that value. I then iterate over the groups, passing each 
vector to the helper function. The tuple returned by the helper only contains 
the aggregate value, so I create a new tuple with fields for the group and the
aggregate value, and insert it into the output file. 

## Join

I know there are much better ways to implement joins, but I went with a nested 
loop join. The code is nearly identical for all join predicates, save for the 
comparison operator, and does the following:
For each tuple in the left table, iterate over all tuples in the right table 
and check the join condition (using the tuple descriptors to find the field 
indexes for the fields specified in the join predicate). When a pair that 
passes the predicate is found, I create a vector of fields by inserting all 
fields from the left tuple, then all from the right. This vector is used to 
construct a new tuple that is added to the output file. 
The only case that differs slightly is equality, where I added a check to 
account for the case where the field used for the join is present in both the 
left and the right files. In this case the join field is skipped when adding 
fields from the right file to the output tuple in order to avoid duplication. 

## ColumnStats

### constructor

I added private members to the ColumnStats object to hold: number of buckets, 
min, max, width, and total count of values, as well as a vector for the 
histogram itself. The constructor sets min, max, and buckets to their passed in 
values, calculates and sets the width, initializes the total to 0 and sizes the 
histogram vector based on the number of buckets. 

### addValue

I added a private method to the ColumnStats class called getIndex, which takes 
in a value and returns the index of the histogram bucket it falls within. If 
the value is outside of the min,max range, an out of range error is thrown. 

assValue uses getIndex to calculate the appropriate index, and increments both 
the total value count and the value in the histogram vector at the index. 

### estimateCardinality 

To estimate the cardinality, I use getIndex to find the bucket that the value 
passed in with the predicate falls into. Because I implemented getIndex to 
throw an exception for values smaller than min or greater than max, I had to 
insert a check such that if the value v passed in for comparison is smaller 
than min or greater than max, it is set to min or max accordingly. 

I used a switch statement on the predicate op to calculate the estimates as 
described in the assignment README. 

I ran into some trouble with the calculations for LT and GT. 
Based on the README, the fraction of the bucket that is greater than is 
((r - v) / w), thus the number of values in that bucket estimated to be 
greater would be: 
((r - v) / w) * height. 
When I tried this I was failing tests, and on looking at the test code it seems
to actually expect:
(height * (r - v)) / w 
which gives a different result.   
  
# Time Spent and Collaboration
I worked on this assignment over the course of a week, spending about 4 full 
days writing and debugging my code. My time was split pretty evenly between the 
Query functions and the Cardinality estimation, with about 2 days on each. I 
did not work with a partner or collaborate on this assignment. 

# Questions

1. The problem with this approach is that any tuple that satisfies BOTH of the 
predicates will be duplicated in the output file. Using three filter operations 
with the same input and output file could solve this: Assume the predicates are 
P1 and P2, to implement P1 OR P2:
	1. filter for P1 AND (!P2)
	2. filter for P2 AND (!P1)
	3. filter for P1 AND P2

2. In my implementation I use an unordered map to group tuples based on the 
value in a single 'group by' field. The key to this map can be extended to be 
multiple values from multiple fields to allow grouping by multiple fields. To 
implement having, I could check the having predicate on each tuple before it 
is added to the map, discarding those that dont satisfy it. 

3. The complexity of my join operation is exponential - O(n^2). Implementing a 
Sort Merge Join could reduce this significantly. 

4. In order to estimate the I/O cost of the query, we would need more 
information on how the data is stored, i.e. heap vs. sorted files, whether
there is an index on the predicate key, is the index clustered or unclustered, 
wtc. 

5. Based on the histograms of the join columns, we could estimate the 
cardinality of the join predicate for each table and multiply the two values 
to estimate the cardinality of the join. 

6. Since the cardinality is 1000, and there are 50 tuples per page, if it is 
stored in a B+ Tree we expect to access 1000/50 = 20 leaf pages. To get to the 
first leaf page we need to traverse the root and internal levels of the tree, 
but we will not have to traverse the tree again to read the next 19 pages so 
the total pages read would be 22. 
If it is stored in a heap file, we would have to traverse all 150000 pages. 
