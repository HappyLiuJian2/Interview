#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

using namespace std;

typedef struct Row {
  int a;
  int b;
} Row;

// Table is a simple wrapper of Rows
struct Table {
  const Row *rows;
  const int nrows;
};

// The ResultSet represents the result set of a query, containing only the row
// number of the result. ResultSet supports composition, including intersection
// and union. Output is also supported.
struct ResultSet {
  vector<int> result;
  ResultSet(vector<int> result) : result(result) {}
  enum class CombineType { UNION, INTERSECTION };
  ResultSet &Combine(const ResultSet &other, CombineType type) {
    vector<int> ret;
    switch (type) {
      case CombineType::INTERSECTION:
        set_intersection(result.begin(), result.end(), other.result.begin(),
                         other.result.end(), back_inserter(ret));
        break;
      case CombineType::UNION:
        set_union(result.begin(), result.end(), other.result.begin(),
                         other.result.end(), back_inserter(ret));
        break;
      default:
        break;
    }
    this->result = ret;
    return *this;
  }
  ResultSet &Intersection(const ResultSet &other) {
    return Combine(other, CombineType::INTERSECTION);
  }
  ResultSet &Union(const ResultSet &other) {
    return Combine(other, CombineType::UNION);
  }
  void Print(Table table) {
    for (auto row : result) {
      cout << table.rows[row].a << ", " << table.rows[row].b << endl;
    }
  }
};

// Query Conditions are simple wrapper of int & pair
using EqualQuery = int;
using RangeQuery = pair<int, int>;

// RBTreeIndex provides a tree index, using the multimap provided by stl to
// build a red-black tree index. Supports equal query and range query, and the
// query results can be sorted according to the original data or index
template <const int N>
struct RBTreeIndex {
  Table table;
  multimap<int, int> index;
  RBTreeIndex(Table t) : table(t) {
    for (int i = 0; i < table.nrows; i++) {
      if (N == 0) {
        index.emplace(table.rows[i].a, i);
      } else {
        index.emplace(table.rows[i].b, i);
      }
    }
  }
  ResultSet apply_equal_query(EqualQuery cond, bool sort_by_index = true) {
    vector<int> ret;
    auto cond_fit__rows = index.equal_range(cond);
    for (auto it = cond_fit__rows.first; it != cond_fit__rows.second; it++) {
      ret.emplace_back(it->second);
    }
    if (sort_by_index) {
      sort(ret.begin(), ret.end());
    }
    return ret;
  }
  ResultSet apply_range_query(RangeQuery cond, bool sort_by_index = true) {
    vector<int> ret;
    auto cond_fit__lower = index.lower_bound(cond.first);
    auto cond_fit__upper = index.lower_bound(cond.second);
    for (auto it = cond_fit__lower; it != cond_fit__upper; it++) {
      ret.emplace_back(it->second);
    }
    if (sort_by_index) {
      sort(ret.begin(), ret.end());
    }
    return ret;
  }
};

// DirectIndex is a simulated index structure, when the input is ordered,
// through this structure can be directly binary lookup, does not occupy
// additional index space
struct DirectIndex {
  Table table;
  DirectIndex(Table t) : table(t) {}
  ResultSet apply_equal_query(EqualQuery cond, bool sort_by_index = true) {
    vector<int> ret;
    auto cond_fit__lower =
        lower_bound(table.rows, table.rows + table.nrows, cond,
                    [](const Row row, int c) { return row.a < c; });
    auto cond_fit__upper =
        upper_bound(table.rows, table.rows + table.nrows, cond + 1,
                    [](int c, const Row row) { return c < row.a; });
    for (auto it = cond_fit__lower; it != cond_fit__upper; it++) {
      ret.emplace_back(it - table.rows);
    }
    if (sort_by_index) {
      sort(ret.begin(), ret.end());
    }
    return ret;
  }
};

// UnionIndex represents a composite index of multiple pieces of data. Mainly to
// solve the output sorted by b. The outer index corresponds to b, and the inner
// index corresponds to a. According to the internal index, it is divided into a
// tree index and a hash index
template <typename Inner>
struct UnionIndex {
  Table table;
  map<int, Inner> index;
  UnionIndex(Table t) : table(t) {
    for (int i = 0; i < table.nrows; i++) {
      int column_left = table.rows[i].b;
      int column_right = table.rows[i].a;
      if (!index.count(column_left)) {
        Inner sub_index;
        sub_index.emplace(column_right, i);
        index.emplace(column_left, sub_index);
      } else {
        index[column_left].emplace(column_right, i);
      }
    }
  }
  ResultSet apply_union_query(RangeQuery cond1, vector<EqualQuery> cond2) {
    vector<int> ret;
    auto cond_fit__lower = index.lower_bound(cond1.first);
    auto cond_fit__uppper = index.lower_bound(cond1.second);
    for (auto it = cond_fit__lower; it != cond_fit__uppper; it++) {
      for (auto cond : cond2) {
        auto cond_fit__rows = it->second.equal_range(cond);
        for (auto it2 = cond_fit__rows.first; it2 != cond_fit__rows.second;
             it2++) {
          ret.emplace_back(it2->second);
        }
      }
    }
    return ret;
  }
};

using UnionRBTreeIndex = UnionIndex<multimap<int, int>>;
using UnionHashIndex = UnionIndex<unordered_multimap<int, int>>;

// The main idea of Task1 is:
//  1. build tree index on a and b
//  2. query the conditions of a and b in the index
//  3. combine the results together according to the combination relationship
//  4. and finally output
void task1(const Row *rows, int nrows) {
  Table t = {rows, nrows};

  RBTreeIndex<0> index_a(t);
  RBTreeIndex<1> index_b(t);

  auto result_set =
      index_a.apply_equal_query(1000)
          .Union(index_a.apply_equal_query(2000))
          .Union(index_a.apply_equal_query(3000))
          .Intersection(index_b.apply_range_query(make_pair(10, 50)));

  result_set.Print(t);
}

// Task2 is updated, and column a is entered sequentially. The main optimization
// ideas are:
//  1. build a linear index on a (which does not actually occupy space), and
//  still build a tree index on b
//  2. query the conditions of a and b in the index
//  3. combine the results together according to the combination relationship
//  4. finally output
void task2(const Row *rows, int nrows) {
  Table t = {rows, nrows};

  DirectIndex index_a(t);
  RBTreeIndex<1> index_b(t);

  auto result_set =
      index_a.apply_equal_query(1000)
          .Union(index_a.apply_equal_query(2000))
          .Union(index_a.apply_equal_query(3000))
          .Intersection(index_b.apply_range_query(make_pair(10, 50)));

  result_set.Print(t);
}

// Task3 is updated, requires the results to be output in column b order. The
// main optimization ideas are:
//  1. build a union index on b and a
//  2. first query the condition of b
//  3. among the results, query the condition of a
//  4. finally output
void task3(const Row *rows, int nrows) {
  Table t = {rows, nrows};

  UnionRBTreeIndex index(t);

  auto result_set =
      index.apply_union_query(make_pair(10, 50), {1000, 2000, 3000});

  result_set.Print(t);
}

// Task4 is updated, a's point query criteria are more numerous. The main
// optimization ideas are:
//  1. build a union index on b and a, in which subindex of a is a hashindex
//  2. first query the condition of b
//  3. among the results, query the condition of a
//  4. finally output
void task4(const Row *rows, int nrows) {
  Table t = {rows, nrows};

  UnionHashIndex index(t);

  auto result_set =
      index.apply_union_query(make_pair(10, 50), {1000, 2000, 3000});

  result_set.Print(t);
}

// test case
void testcase1(function<void(const Row *, int)> task) {
  cout << "===================testcase1 begin======================" << endl;
  Row rows[4] = {{1000, 10}, {2000, 40}, {3000, 90}, {4000, 20}};
  task(rows, 4);
  cout << "===================testcase1 end========================" << endl;
}

void testcase2(function<void(const Row *, int)> task) {
  cout << "===================testcase2 begin======================" << endl;
  vector<Row> rows;
  for (int i = 1000; i <= 5000; i += 1000) {
    for (int j = 0; j < 100; j++) {
      rows.push_back(Row{i, j});
    }
  }
  task(&rows[0], rows.size());
  cout << "===================testcase2 end========================" << endl;
}

void testcase3(function<void(const Row *, int)> task) {
  cout << "===================testcase3 begin======================" << endl;
  vector<Row> rows;
  for (int i = 1000; i <= 5000; i += 1000) {
    for (int j = 100; j > 0; j--) {
      rows.push_back(Row{i, j});
    }
  }
  task(&rows[0], rows.size());
  cout << "===================testcase3 end========================" << endl;
}

int main() {
  testcase1(task1);
  testcase2(task1);
  testcase3(task1);
  testcase1(task2);
  testcase2(task2);
  testcase3(task2);
  testcase1(task3);
  testcase2(task3);
  testcase3(task3);
  testcase1(task4);
  testcase2(task4);
  testcase3(task4);
  return 0;
}