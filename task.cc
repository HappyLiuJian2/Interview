#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <unordered_map>

using namespace std;

typedef struct Row {
    int a;
    int b;
} Row;

struct Table
{
    const Row* rows;
    const int nrows;
};

struct ResultSet {
    vector<int> result;
    ResultSet(vector<int> result) : result(result) {}
    ResultSet& Intersection(const ResultSet& other) {
        vector<int> ret;
        set_intersection(result.begin(), result.end(), other.result.begin(), other.result.end(), back_inserter(ret));
        this->result = ret;
        return *this;
    }
    ResultSet& Union(const ResultSet& other) {
        vector<int> ret;
        set_union(result.begin(), result.end(), other.result.begin(), other.result.end(), back_inserter(ret));
        this->result = ret;
        return *this;
    }
    void Print(Table table) {
        for (auto row: result) {
            cout << table.rows[row].a << ", " << table.rows[row].b << endl;
        }
    }
};

using EqualQuery = int;
using RangeQuery = pair<int, int>;

template<const int N>
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

struct DirectIndex {
    Table table;
    DirectIndex(Table t) : table(t) {}
    ResultSet apply_equal_query(EqualQuery cond, bool sort_by_index = true) {
        vector<int> ret;
        auto cond_fit__lower = lower_bound(table.rows, table.rows + table.nrows, cond, [](const Row row, int c) {
            return row.a < c;
        });
        auto cond_fit__upper = upper_bound(table.rows, table.rows + table.nrows, cond + 1, [](int c, const Row row) {
            return c < row.a;
        });
        for (auto it = cond_fit__lower; it != cond_fit__upper; it++) {
            ret.emplace_back(it - table.rows);
        }
        if (sort_by_index) {
            sort(ret.begin(), ret.end());
        }
        return ret;
    }
};

struct UnionIndex {
    Table table;
    map<int, multimap<int, int>> index;
    UnionIndex(Table t) : table(t) {
        for (int i = 0; i < table.nrows; i++) {
            int column_left = table.rows[i].b;
            int column_right = table.rows[i].a;
            if (!index.count(column_left)) {
                multimap<int, int> sub_index;
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
            for (auto cond: cond2) {
                auto cond_fit__rows = it->second.equal_range(cond);
                for (auto it2 = cond_fit__rows.first; it2 != cond_fit__rows.second; it2++) {
                    ret.emplace_back(it2->second);
                }
            }
        }
        return ret;
    }
};

struct UnionHashIndex {
    Table table;
    map<int, unordered_multimap<int, int>> index;
    UnionHashIndex(Table t) : table(t) {
        for (int i = 0; i < table.nrows; i++) {
            int column_left = table.rows[i].b;
            int column_right = table.rows[i].a;
            if (!index.count(column_left)) {
                unordered_multimap<int, int> sub_index;
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
            for (auto cond: cond2) {
                auto cond_fit__rows = it->second.equal_range(cond);
                for (auto it2 = cond_fit__rows.first; it2 != cond_fit__rows.second; it2++) {
                    ret.emplace_back(it2->second);
                }
            }
        }
        return ret;
    }
};

void task1(const Row* rows, int nrows) {
    Table t = {rows, nrows};

    RBTreeIndex<0> index_a(t);
    RBTreeIndex<1> index_b(t);

    auto result_set = index_a.apply_equal_query(1000).Union(index_a.apply_equal_query(2000)).Union(index_a.apply_equal_query(3000)).Intersection(index_b.apply_range_query(make_pair(10, 50)));

    result_set.Print(t);
}

void task2(const Row* rows, int nrows) {
    Table t = {rows, nrows};

    DirectIndex index_a(t);
    RBTreeIndex<1> index_b(t);

    auto result_set = index_a.apply_equal_query(1000).Union(index_a.apply_equal_query(2000)).Union(index_a.apply_equal_query(3000)).Intersection(index_b.apply_range_query(make_pair(10, 50)));

    result_set.Print(t);
}

void task3(const Row* rows, int nrows) {
    Table t = {rows, nrows};

    UnionIndex index(t);

    auto result_set = index.apply_union_query(make_pair(10, 50), {1000, 2000, 3000});

    result_set.Print(t);
}

void task4(const Row* rows, int nrows) {
    Table t = {rows, nrows};

    UnionHashIndex index(t);

    auto result_set = index.apply_union_query(make_pair(10, 50), {1000, 2000, 3000});

    result_set.Print(t);
}

// test case
void task1_testcase1() {
    cout << "===================task1 testcase1 begin======================" << endl;
    Row rows[3] = {{1000,10}, {2000,40}, {1500, 30}};
    task1(rows, 3);
    cout << "===================task1 testcase1 end========================" << endl;
}

void task1_testcase2() {
    cout << "===================task1 testcase2 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 0; j < 100; j++) {
            rows.push_back(Row{i,j});
        }
    }
    task1(&rows[0], rows.size());
    cout << "===================task1 testcase2 end========================" << endl;
}

void task1_testcase3() {
    cout << "===================task1 testcase3 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 100; j > 0; j--) {
            rows.push_back(Row{i,j});
        }
    }
    task1(&rows[0], rows.size());
    cout << "===================task1 testcase3 end========================" << endl;
}

// task2
void task2_testcase1() {
    cout << "===================task2 testcase1 begin======================" << endl;
    Row rows[4] = {{1000,10}, {2000,40}, {3000, 90}, {4000, 20}};
    task2(rows, 4);
    cout << "===================task2 testcase1 end========================" << endl;
}

void task2_testcase2() {
    cout << "===================task2 testcase2 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 0; j < 100; j++) {
            rows.push_back(Row{i,j});
        }
    }
    task2(&rows[0], rows.size());
    cout << "===================task2 testcase2 end========================" << endl;
}

void task2_testcase3() {
    cout << "===================task2 testcase3 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 100; j > 0; j--) {
            rows.push_back(Row{i,j});
        }
    }
    task2(&rows[0], rows.size());
    cout << "===================task2 testcase3 end========================" << endl;
}

// task3
void task3_testcase1() {
    cout << "===================task3 testcase1 begin======================" << endl;
    Row rows[4] = {{1000,10}, {2000,40}, {3000, 90}, {4000, 20}};
    task3(rows, 4);
    cout << "===================task3 testcase1 end========================" << endl;
}

void task3_testcase2() {
    cout << "===================task3 testcase2 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 0; j < 100; j++) {
            rows.push_back(Row{i,j});
        }
    }
    task3(&rows[0], rows.size());
    cout << "===================task3 testcase2 end========================" << endl;
}

void task3_testcase3() {
    cout << "===================task3 testcase3 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 100; j > 0; j--) {
            rows.push_back(Row{i,j});
        }
    }
    task3(&rows[0], rows.size());
    cout << "===================task3 testcase3 end========================" << endl;
}

// task4
void task4_testcase1() {
    cout << "===================task4 testcase1 begin======================" << endl;
    Row rows[4] = {{1000,10}, {2000,40}, {3000, 90}, {4000, 20}};
    task4(rows, 4);
    cout << "===================task4 testcase1 end========================" << endl;
}

void task4_testcase2() {
    cout << "===================task4 testcase2 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 0; j < 100; j++) {
            rows.push_back(Row{i,j});
        }
    }
    task4(&rows[0], rows.size());
    cout << "===================task4 testcase2 end========================" << endl;
}

void task4_testcase3() {
    cout << "===================task4 testcase3 begin======================" << endl;
    vector<Row> rows;
    for (int i = 1000; i <= 5000; i+=1000) {
        for (int j = 100; j > 0; j--) {
            rows.push_back(Row{i,j});
        }
    }
    task4(&rows[0], rows.size());
    cout << "===================task4 testcase3 end========================" << endl;
}

int main() {
    task1_testcase1();
    task1_testcase2();
    task1_testcase3();
    task2_testcase1();
    task2_testcase2();
    task2_testcase3();
    task3_testcase1();
    task3_testcase2();
    task3_testcase3();
    task4_testcase1();
    task4_testcase2();
    task4_testcase3();
    return 0;
}