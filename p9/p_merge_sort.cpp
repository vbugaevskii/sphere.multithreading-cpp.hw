#include <iostream>
#include <cstring>

template<class T> void merge(T *a, int l_border, int m_border, int r_border)
{
    int i = l_border, j = m_border + 1, k = 0;
    T tmp[r_border-l_border+1];

    for (; i <= m_border && j <= r_border; k++)
        tmp[k] = (a[i] < a[j]) ? a[i++] : a[j++];

    if (m_border - i + 1)
    {
        memcpy(&tmp[k], &a[i], (m_border - i + 1) * sizeof(T));
        k += m_border - i + 1;
    }

    if (r_border - j + 1)
    {
        memcpy(&tmp[k], &a[j], (r_border - j + 1) * sizeof(T));
        k += r_border - j + 1;
    }

    memcpy(&a[l_border], tmp, k * sizeof(T));
}

template<class T> void mergesort(T *a, int l_border, int r_border)
{
    if (l_border < r_border)
    {
        int m_border = (l_border + r_border) / 2;

        #pragma omp task firstprivate (a, l_border, m_border)
        {
            mergesort(a, l_border, m_border);
        }

        #pragma omp task firstprivate (a, m_border, r_border)
        {
            mergesort(a, m_border+1, r_border);
        }

        #pragma omp taskwait
        {
            merge(a, l_border, m_border, r_border);
        }
    }
}

int main()
{
    int a[int(1e6)];

    srand(time(NULL));

    for (int i = 0; i < 1e6; i++)
        a[i] = rand() % 1000;

    clock_t tick = clock();
    #pragma omp parallel num_threads(4)
    {
        #pragma omp single
        {
            mergesort<int>(a, 0, 1e6 - 1);
        }
    }
    clock_t tock = clock();

    std::cout << tock - tick << std::endl;

    return 0;
}
