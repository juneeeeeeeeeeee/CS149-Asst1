# sqrt에서 ISPC 속도 향상
```
[sqrt serial]:          [838.698] ms
[sqrt ispc]:            [205.372] ms
[sqrt task ispc]:       [44.915] ms
                                (4.08x speedup from ISPC)
                                (18.67x speedup from task ISPC)
```
이때 실행하는 CPU는 intel의 Tiger Lake 아키텍쳐로, core 안에 2개의 8-wide vector ALU(256bit AVX2)가 들어있다. 따라서 이상적으로는 SIMD 병렬화로 총 16배의 speedup을 기대할 수 있다. 실제로는 이것에 한참 못 미치는 4.08배의 speedup을 얻었다. 그 이유는 gang 내부에서 inverse square root 값에 수렴하기 위한 iteration이 각 값들마다 차이가 있어 bottleneck 현상이 발생하기 때문이다. 

![img](../handout-images/sqrt_graph.jpg)

한편 4-core 2-thread에서 64개의 task ISPC를 생성한 경우 앞서 설명한 것처럼 각각의 task가 걸리는 시간이 달라 worker thread에 어느 정도 균등하게 분배된다. 따라서 이상적인 speedup은 64배이다. 실제 speedup은 18.67배이다. 

task를 생성한 경우가 task를 생성하지 않은 경우에 비해 4배 이상의 speedup을 보여주는데, 이는 task를 각각의 core에 균등하게 분배하여 얻은 speedup으로 보인다. 그러나 이상적인 speedup에 비하면 여전히 적은 값인데 이는 위와 동일하게 gang 내부의 iteration 차이 때문으로 생각된다. 

# ISPC 구현의 상대적 속도 향상을 위한 `values` 배열 수정
앞에서 설명한 바와 같이 `values` 배열의 값이 동일하면 gang 내부나 worker thread 간에 부하 불균형이 없어 좋은 속도 향상이 있을 것이라 생각할 수 있다. 이때 두 가지 경우를 실험해보았다. 

우선 모든 `values`가 1인 경우(iteration 1번에 모두 종료된다. )의 speedup은 아래와 같다. 
```
[sqrt serial]:          [17.579] ms
[sqrt ispc]:            [8.326] ms
[sqrt task ispc]:       [4.376] ms
                                (2.11x speedup from ISPC)
                                (4.02x speedup from task ISPC)
```
이때 speedup이 기대하는 것만큼 나오지 않는 것을 확인할 수 있다. 이는 ISPC 병렬화를 위한 overhead에 비해 병렬화된 부분이 너무 짧아서 발생하는 문제이다. 

다음으로 모든 `values`가 2.999인 경우(iteration 횟수가 많다. )의 speedup은 아래와 같다. 
```
[sqrt serial]:          [1652.298] ms
[sqrt ispc]:            [305.294] ms
[sqrt task ispc]:       [54.880] ms
                                (5.41x speedup from ISPC)
                                (30.11x speedup from task ISPC)
```
이때 기존의 random한 `values`에 비해 ISPC speedup이 더 높음을 확인할 수 있다. 이는 gang 내부에서 workload가 잘 밸런싱되어 있고, 또한 iteration이 많아 ISPC 병렬화를 위한 overhead가 상대적으로 작아진다. 

한편 기존에 비해 task speedup도 높음을 확인할 수 있다. 기존의 random한 `values`에서는 task를 아무리 잘 나누어도 worker thread 간의 workload 불균형 문제가 발생할 수 밖에 없는데 이 case에서는 이를 잘 밸런싱하고, 또한 iteration이 많아 worker thread에 task를 할당하는 등의 overhead가 상대적으로 작아진다. 

# ISPC의 속도 향상을 최소화하는 `values` 배열
gang 내부에 하나의 index만 연산이 오래 걸리는 경우 ISPC speedup이 작을 것으로 예측할 수 있다. 따라서 `values`의 index가 8의 배수인 경우 2.999, 그렇지 않은 경우 1로 설정하였다. 이때의 speedup은 아래와 같다. 

```
[sqrt serial]:          [288.454] ms
[sqrt ispc]:            [346.607] ms
[sqrt task ispc]:       [65.877] ms
                                (0.83x speedup from ISPC)
                                (4.38x speedup from task ISPC)
```
예상하는 결과와 일치함을 확인할 수 있다. (심지어 speedup이 1보다도 작다. )한편 task를 생성하는 경우에는 8개의 thread간에 load 밸런싱이 잘 맞아 speedup이 ISPC에 비해서는 잘 나옴을 확인할 수 있다. 
