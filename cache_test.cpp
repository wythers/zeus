// Copyright (C) 2021- 

#define ZEUS_LIBRARY_DEBUG
#include "cache.hpp"

#include <iostream>

#define PASSED 0
#define N  10 << 10

using namespace zeus;
using namespace std;
struct tester {
        atomic_ulong& cnt;
        tester(atomic<ulong>& c) : cnt(c)  {
                cnt++;
        }
        ~tester() {
                cnt--;
        }
};

int test_cache_case1(zeus::Pool<tester, 16>&);
int test_cache_case2(zeus::Pool<tester, 16>&);
int test_cache_case3(zeus::Pool<tester, 16>&);
int test_cache_case4(zeus::Pool<tester, 16>&);

using Case = int (*)(zeus::Pool<tester, 16>&);

/**
 * zeus::pool unit test.
*/

int main() {
		zeus::Pool<tester, 16> pool{};
        Case  cases[4]{
                        test_cache_case1,
                        test_cache_case2,
                        test_cache_case3,
                        test_cache_case4
                };

        char const* hdr = "zeus::pooll test case #";
        for (int i = 0; i < 4; i++) {
                int ret = cases[i](pool);
                printf( "=== RUN %s%d "
                        "--------------"
                        "-------------- %s\n", 
                        hdr, i+1,
                        ret == PASSED ? "passed" : "fail");
        }

        return 0;
}

int test_cache_case1(zeus::Pool<tester, 16>& pool) {
        atomic_ulong cnt = 0;
        vector<thread> ths{};
        atomic_bool flag = false;
        auto [Vcnt, Ccnt] = decay_t<decltype(pool)>::ZeusUnitMonitor();

        for (uint i = 0; i < 8; i++) {
                ths.emplace_back(thread([&]{
                        auto* p = pool.Get(cnt);
                        flag.wait(false);
                        pool.Put(p);
                }));
        }

        flag = true;
        flag.notify_all();
        for (auto& th : ths) {
                th.join();
        }

        return cnt.load() || Vcnt.load() || Ccnt.load();
}

// one to one
int test_cache_case2(zeus::Pool<tester, 16>& pool) {
        auto [Vcnt, Ccnt] = decay_t<decltype(pool)>::ZeusUnitMonitor();
        atomic_ulong cnt{}; 
        atomic_bool flag{};  

        thread th1([&]{
                for (ulong i = 0; i < N; i++) {
                        pool.Put(new tester{cnt});
                }
                flag = true;
        });
        
        for (;;) {
                auto* p = pool.Get(cnt);
                if (!p && !flag)
                        continue;
                if (p) {
                        delete p;
                } else {
                        break;
                }
        }

        th1.join();
        return cnt.load() || Vcnt.load() || Ccnt.load();
}

// many to one
int test_cache_case3(zeus::Pool<tester, 16>& pool) {
        atomic_ulong cnt = 0;
        atomic_uint cnt2 = 0;
        vector<thread> ths{};
        
        auto [Vcnt, Ccnt] = decay_t<decltype(pool)>::ZeusUnitMonitor();

        for (uint i = 0; i < 4; i++) {
                ths.emplace_back(thread([&]{
                        for (ulong j = 0; j < N / 4; j++) {
                                pool.Put(new tester(cnt));
                        }
                        cnt2++;
                }));
        }

        for (;;) {
                auto* p = pool.Get(cnt);
                if (p) 
                        delete p;
                if (cnt2 == 4)
                        break;
        }

        for (auto& th : ths) {
                th.join();
        }

        return cnt.load() || Vcnt.load() || Ccnt.load();
}

// many to many
int test_cache_case4(zeus::Pool<tester, 16>& pool) {
        atomic_ulong cnt = 0;
        atomic_uint cnt2 = 0;
        vector<thread> ths{};
        
        auto [Vcnt, Ccnt] = decay_t<decltype(pool)>::ZeusUnitMonitor();

        for (uint i = 0; i < 8; i++) {
                ths.emplace_back(thread([&]{
                        for (ulong j = 0; j < N / 8; j++) {
                                pool.Put(new tester(cnt));
                        }
                        cnt2++;
                }));
        }

        for (uint i = 0; i < 8; i++) {
                ths.emplace_back(thread([&]{
                        for (;;) {
                                auto* p = pool.Get(cnt);
                                if (p) 
                                        delete p;
                                if (cnt2 == 8)
                                        break;
                        }
                }));
        }

        for (auto& th : ths) {
                th.join();
        }

        return cnt.load() || Vcnt.load() || Ccnt.load();
}