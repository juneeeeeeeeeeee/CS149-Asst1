# saxpy에서 ISPC 속도 향상
```
[saxpy serial]:         [12.471] ms     [23.897] GB/s   [3.207] GFLOPS
[saxpy ispc]:           [12.171] ms     [24.487] GB/s   [3.287] GFLOPS
[saxpy task ispc]:      [5.647] ms      [52.773] GB/s   [7.083] GFLOPS
                                (2.16x speedup from use of tasks)
```
ispc를 이용할 때는 speedup이 그렇게 크지 않았다. ispc에 64개의 task를 조합한 경우 memory에서의 BW와 연산량 모두 2배 가까이 늘어났다. 이때 ispc를 이용할 때 speedup이 거의 없는 것으로 보아 연산이 memory bottleneck이 있는 것으로 보인다. 이때 실행하는 CPU는 intel의 Tiger Lake 아키텍쳐로, 4 core 8 thread 구조이다. 현재 ispc에 task를 조합한 경우 하나의 core 안의 2개의 thread에서 서로 memory stall을 감추어 speedup의 효과를 본 것으로 보인다. 

# saxpy의 속도 개선 가능성
prog4처럼 ISPC 대신 Intel AVX2 Intrinsics를 이용하면 속도가 더 빨라질 것으로 보인다. 그러나 연산보다 memory BW를 늘려야 하는데, 이를 위해서는 intel CPU의 memory prefetch intrinsic을 이용하면 될 것으로 보인다. 그러나 memory bounded된 시스템에서 memory BW는 제한적이기에 선형적인 속도의 개선은 불가능하다. 

한편 memory prefetch intrinsic은 xmmintrin.h 헤더 파일에 포함되어 있는데 이게 ispc랑 같이 쓰기가 어렵다.....포기

# `TOTAL_BYTES = 4 * N * sizeof(float);`인 이유
`result[i] = scale * X[i] + Y[i];`

X[i]는 read only이다. X[i]를 읽을 때 X[i]가 포함된 캐시 라인 전체를 메인 메모리에서 cache로 가져온다. intel의 tiger lake architecture에서 cache line은 64byte이다. 하나의 float는 4byte이므로 X[i] 하나를 읽으려 할 때 16개의 float 값들(X[i]부터 X[i+15]까지)이 한꺼번에 cache로 로드된다. 

Y[i]도 read only이므로 X[i]와 동일하다. 

한편 result[i]의 경우 CPU는 result[i]의 값을 직접 memory에 쓰는 것이 아니라, result[i]가 포함된 cache line을 먼저 cache로 가져온 다음, 해당 cache line의 result[i] 위치만 업데이트한다. 이 cache line은 dirty 상태가 되고, 나중에 다시 memory로 쓰여야 한다. 따라서, result[i]를 업데이트하기 위해 64byte의 cache line을 memory에서 읽어와야 하고, 나중에 이 수정된 64byte cache line을 memory로 써야 한다.

즉 memory 접근은 한 cache line당 4번이며, 전체 memory 접근 byte는 `4 * N * sizeof(float)`이다. 