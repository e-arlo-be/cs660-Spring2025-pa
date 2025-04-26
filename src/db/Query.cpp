#include <db/Query.hpp>
#include <stdexcept>

using namespace db;
/* helper functions to simplify aggregate and filter*/
static Tuple aggregate_tuples(const std::vector<Tuple> &in, const Aggregate &agg, const TupleDesc &td);
static std::vector<Tuple> filter_tuples(const DbFile &in, const std::vector<FilterPredicate> &predicates); 

void db::projection(const DbFile &in, DbFile &out, const std::vector<std::string> &field_names) {
    /* using the tuple descriptor, create a vertex of the indices to project */
    std::vector<size_t> indices;
    for (const auto &name: field_names) {
        indices.push_back(in.getTupleDesc().index_of(name));
    }
    /* iterate over the tuples in the input file and create a vector with the fields 
    that correspond to the index vector */
    for (const auto &tup: in) {
        std::vector<field_t> fields;
        for (const auto &index: indices) {
            fields.push_back(tup.get_field(index));
        }
        /* create a new tuple with the selected fields and insert it into the output file */
        out.insertTuple(Tuple(fields));
    }
}

void db::filter(const DbFile &in, DbFile &out, const std::vector<FilterPredicate> &pred) {
    /* filtered_tuples returns a vector of the tuples that match predicate */
    std::vector<Tuple> filtered_tuples = filter_tuples(in, pred);

    /* iterate over the filtered tuples and insert them into the output file */
    for (const auto &tuple : filtered_tuples) {
        out.insertTuple(tuple);
    }
}

void db::aggregate(const DbFile &in, DbFile &out, const Aggregate &agg) {
    /* if there is a 'group by', unordered map is used to sort tuples into vectors 
    based on each tuples value in the 'group by' field */
    if (agg.group) {
        std::unordered_map<field_t, std::vector<Tuple>> groups;
        for (const auto &tuple : in) {
            field_t group_value = tuple.get_field(in.getTupleDesc().index_of(*agg.group));
            groups[group_value].push_back(tuple);
        }
        /* iterate over the groups and apply the aggregate function to each group, since the
        helper function returns a tuple with a single field holding the aggregate value, a new tuple
        is created with the value of the group by field and the aggregate result, and this new tuple
        is inserted into the output file */
        for (const auto &[group_value, tuples] : groups) {
            Tuple result = aggregate_tuples(tuples, agg, in.getTupleDesc());
            std::vector<field_t> fields;
            fields.push_back(group_value);
            fields.push_back(result.get_field(0));
            out.insertTuple(Tuple(fields));
        }
    /* if there is no 'group by', we simply create a vector of all tuples and call the aggregate 
    helper function, and output the resulting tuple to the out file */
    } else {
        std::vector<Tuple> tuples;
        for (const auto &tuple : in) {
            tuples.push_back(tuple);
        }
        Tuple result = aggregate_tuples(tuples, agg, in.getTupleDesc());
        out.insertTuple(result);
    }
}

void db::join(const DbFile &left, const DbFile &right, DbFile &out, const JoinPredicate &pred) {
    switch (pred.op) {
        case PredicateOp::EQ:
            /* for each tuple in the left table, iterate over all tuples in the right table checking
            if the predicate fields match */
            for (const auto &left_tuple : left) {
                for (const auto &right_tuple : right) {
                    if (left_tuple.get_field(left.getTupleDesc().index_of(pred.left)) ==
                        right_tuple.get_field(right.getTupleDesc().index_of(pred.right))) {
                        std::vector<field_t> fields;
                        /* when match is found, create a new vector with the fields of the left tuple 
                        followed by the fields of the right tuple. if the field used for the join is 
                        present in both tuples, it is ignored when adding the fields from the right tuple.
                        the new tuple is added to the output file */
                        for (size_t i = 0; i < left_tuple.size(); i++) {
                            fields.push_back(left_tuple.get_field(i));
                        }
                        for (size_t i = 0; i < right_tuple.size(); i++) {
                            if (i != right.getTupleDesc().index_of(pred.right)) {
                                fields.push_back(right_tuple.get_field(i));
                            }
                        }
                        out.insertTuple(Tuple(fields));
                    }
                }
            }    
        break;
        case PredicateOp::NE:
            /* similar to equal - for each left tuple iterate over all right tuples checking that 
            the predicate fields do not match. if this is the case, add the left then right fields
            to a new tuple and insert it to the output file */
            for (const auto &left_tuple : left) {
                for (const auto &right_tuple : right) {
                    if (left_tuple.get_field(left.getTupleDesc().index_of(pred.left)) !=
                        right_tuple.get_field(right.getTupleDesc().index_of(pred.right))) {
                        std::vector<field_t> fields;
                        for (size_t i = 0; i < left_tuple.size(); i++) {
                            fields.push_back(left_tuple.get_field(i));
                        }
                        for (size_t i = 0; i < right_tuple.size(); i++) {
                            fields.push_back(right_tuple.get_field(i));
                        }
                        out.insertTuple(Tuple(fields));
                    }
                }
            } 
            break;
        case PredicateOp::LT:
            /* same implementation as not equal, only change is the comparison operator */
            for (const auto &left_tuple : left) {
                for (const auto &right_tuple : right) {
                    if (left_tuple.get_field(left.getTupleDesc().index_of(pred.left)) <
                        right_tuple.get_field(right.getTupleDesc().index_of(pred.right))) {
                        std::vector<field_t> fields;
                        for (size_t i = 0; i < left_tuple.size(); i++) {
                            fields.push_back(left_tuple.get_field(i));
                        }
                        for (size_t i = 0; i < right_tuple.size(); i++) {
                            fields.push_back(right_tuple.get_field(i));
                        }
                        out.insertTuple(Tuple(fields));
                    }
                }
            } 
            break;
        case PredicateOp::LE:
            /* same implementation as not equal, only change is the comparison operator */
            for (const auto &left_tuple : left) {
                for (const auto &right_tuple : right) {
                    if (left_tuple.get_field(left.getTupleDesc().index_of(pred.left)) <
                        right_tuple.get_field(right.getTupleDesc().index_of(pred.right))) {
                        std::vector<field_t> fields;
                        for (size_t i = 0; i < left_tuple.size(); i++) {
                            fields.push_back(left_tuple.get_field(i));
                        }
                        for (size_t i = 0; i < right_tuple.size(); i++) {
                            fields.push_back(right_tuple.get_field(i));
                        }
                        out.insertTuple(Tuple(fields));
                    }
                }
            } 
            break;
        case PredicateOp::GT:
            /* same implementation as not equal, only change is the comparison operator */
            for (const auto &left_tuple : left) {
                for (const auto &right_tuple : right) {
                    if (left_tuple.get_field(left.getTupleDesc().index_of(pred.left)) >
                        right_tuple.get_field(right.getTupleDesc().index_of(pred.right))) {
                        std::vector<field_t> fields;
                        for (size_t i = 0; i < left_tuple.size(); i++) {
                            fields.push_back(left_tuple.get_field(i));
                        }
                        for (size_t i = 0; i < right_tuple.size(); i++) {
                            fields.push_back(right_tuple.get_field(i));
                        }
                        out.insertTuple(Tuple(fields));
                    }
                }
            } 
            break;
        case PredicateOp::GE:
            /* same implementation as not equal, only change is the comparison operator */
            for (const auto &left_tuple : left) {
                for (const auto &right_tuple : right) {
                    if (left_tuple.get_field(left.getTupleDesc().index_of(pred.left)) >=
                        right_tuple.get_field(right.getTupleDesc().index_of(pred.right))) {
                        std::vector<field_t> fields;
                        for (size_t i = 0; i < left_tuple.size(); i++) {
                            fields.push_back(left_tuple.get_field(i));
                        }
                        for (size_t i = 0; i < right_tuple.size(); i++) {
                            fields.push_back(right_tuple.get_field(i));
                        }
                        out.insertTuple(Tuple(fields));
                    }
                }
            } 
            break;
    }
    

}
static Tuple aggregate_tuples(const std::vector<Tuple> &in, const Aggregate &agg, const TupleDesc &td) {
    field_t agg_value;
    int index = td.index_of(agg.field);
    std::vector<field_t> fields;
    switch (agg.op) {
    case AggregateOp::SUM:
        /* since field_t is a variant, need to determine the type of the field being summed and 
        initialize agg_value accordingly */
        if (td.field_type(index) == type_t::INT) {
            agg_value = 0;
        }
        else if (td.field_type(index) == type_t::DOUBLE) {
            agg_value = 0.0;
        }
        else {
            throw std::logic_error("Unsupported type for SUM");
        }
        for (const auto &tuple : in) {
            /* both the fields and agg_value are variants, so how we access and modify them 
            is dependent on the type we are working with */
            if (std::holds_alternative<int>(agg_value)) {
                agg_value = std::get<int>(agg_value) + std::get<int>(tuple.get_field(index));
            }
            else {
                agg_value = std::get<double>(agg_value) + std::get<double>(tuple.get_field(index));
            }
        }
        /* add the result to a vector of fields that will be used to create the resulting tuple */
        fields.push_back(agg_value);
        break;
    case AggregateOp::AVG:
        /* avg is the same as sum for the summation, with the addition of a division by the 
        size of the input file in order to calculate the average */
        if (td.field_type(index) == type_t::INT) {
            agg_value = 0;
        }
        else if (td.field_type(index) == type_t::DOUBLE) {
            agg_value = 0.0;
        }
        else {
            throw std::logic_error("Unsupported type for AVG");
        }
        for (const auto &tuple : in) {
            if (std::holds_alternative<int>(agg_value)) {
                agg_value = std::get<int>(agg_value) + std::get<int>(tuple.get_field(index));
            }
            else {
                agg_value = std::get<double>(agg_value) + std::get<double>(tuple.get_field(index));
            }
        }
        if (std::holds_alternative<int>(agg_value)) {
            agg_value = std::get<int>(agg_value) / static_cast<double>(in.size());
        }
        else {
            agg_value = std::get<double>(agg_value) / static_cast<double>(in.size());
        }
        fields.push_back(agg_value);
        break;
    case AggregateOp::COUNT:
        /* count is simply the size of the input file */
        agg_value = static_cast<int>(in.size());
        fields.push_back(agg_value);
        break;
    case AggregateOp::MIN:
        /* store the field of interest from the first tuple in agg_value, compare to the 
        field from each subsequent tuple and update agg_value if we encounter a smaller
        value */
        agg_value = in[0].get_field(index);
        for (const auto &tuple : in) {
            if (tuple.get_field(index) < agg_value) {
                agg_value = tuple.get_field(index);
            }
        }
        fields.push_back(agg_value);
        break;
    case AggregateOp::MAX:
        /* similar to min, checks for larger field values */
        agg_value = in[0].get_field(index);
        for (const auto &tuple : in) {
            if (tuple.get_field(index) > agg_value) {
                agg_value = tuple.get_field(index);
            }
        }
        fields.push_back(agg_value);
        break;
    }
    /* create a new tuple with the aggregate value and return it */
    return Tuple(fields);
}

static std::vector<Tuple> filter_tuples(const DbFile &in, const std::vector<FilterPredicate> &predicates) {
    /* for each tuple in the input file, check all predicates in the predicate vector. if a tuple fails a 
    predicate, we break and move onto the next tuple without checking remaining predicates unnecessarily. 
    the boolean 'match' will only be true when the inner loop breaks if the tuple passed ALL predicates, 
    in which case it is added to the vector of tuples to be returned. */
    std::vector<Tuple> result;
    for (const auto &tuple : in) {
        const TupleDesc &td = in.getTupleDesc();
        bool match = true;
        for (const auto &predicate : predicates) {
            const field_t &field = tuple.get_field(td.index_of(predicate.field_name));
            switch (predicate.op) {
                case PredicateOp::EQ:
                    if (field != predicate.value) {
                        match = false;
                    }
                    break;
                case PredicateOp::NE:
                    if (field == predicate.value) {
                        match = false;
                    }
                    break;
                case PredicateOp::LT:
                    if (field >= predicate.value) {
                        match = false;
                    }
                    break;
                case PredicateOp::LE:
                    if (field > predicate.value) {
                        match = false;
                    }
                    break;
                case PredicateOp::GT:
                    if (field <= predicate.value) {
                        match = false;
                    }
                    break;
                case PredicateOp::GE:
                    if (field < predicate.value) {
                        match = false;
                    }
                    break;
            }
            if (!match) {
                break;
            }
        }
        if (match) {
            result.push_back(tuple);
        }
    }
    return result;
}

