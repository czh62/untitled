#include "algorithm.h"

#include <QDebug>
#include <random>

using namespace std;

// 求(x*y)%n
int64_t mod_mux(int64_t x, int64_t y, int64_t n)
{
    int64_t ret = 0, tmp = x % n;
    while (y)
    {
        if (y & 0x1)
            if ((ret += tmp) > n)
                ret -= n;
        if ((tmp <<= 1) > n)
            tmp -= n;
        y >>= 1;
    }
    return ret;
}

// 模重复平方算法求(a^b) % c
int64_t mod_pow(int64_t a, uint64_t b, int64_t c)
{
    int64_t ret = 1;
    while (b)
    {
        if (b & 0x1)
            ret = mod_mux(ret, a, c);
        a = mod_mux(a, a, c);
        b >>= 1;
    }
    return ret;
}

bool IsPrime(int64_t n)
{
    // millerTest
    int64_t t = 100; // 100次检验
    int64_t k = 0, m, a, i;

    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<int64_t> dis(2, n - 2);

    for (m = n - 1; !(m & 1); m >>= 1, k++)
        ;
    while (t--)
    {
        a = mod_pow(dis(gen), m, n);
        if (a != 1)
        {
            for (i = 0; i < k && a != n - 1; i++)
                a = mod_mux(a, a, n);
            if (i >= k)
                return false;
        }
    }
    return true;
}

int64_t ProduceRandomPrime()
{
    while (true)
    {
        // 生成
        random_device rd;
        mt19937_64 gen(rd());
        uniform_int_distribution<int64_t> dis(0x0000800000000000, 0x0000ffffffffffff);

        int64_t n = dis(gen);
        // 奇数
        n |= 1;
        // 判断
        if (IsPrime(n) && IsPrime(2 * n + 1))
            return 2 * n + 1;
    }
}

int64_t PrimitiveElement(int64_t n)
{
    int64_t p = (n - 1) / 2;
    while (true)
    {
        random_device rd;
        mt19937_64 gen(rd());
        uniform_int_distribution<int64_t> dis(2, n - 2);
        int64_t g;
        do
        {
            g = dis(gen);
        } while (mod_pow(g, 2, n) == 1 || mod_pow(g, p, n) == 1);
        return g;
    }
}
