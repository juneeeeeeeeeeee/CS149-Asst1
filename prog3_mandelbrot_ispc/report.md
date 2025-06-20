# part 1: ISPC basics
해당 코드를 make하고 실행했을 때 실행 시간은 아래와 같다. (multicore ispc도 같이 실행하였다. )
```
[mandelbrot serial]:            [214.127] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot ispc]:              [47.886] ms
Wrote image file mandelbrot-ispc.ppm
[mandelbrot multicore ispc]:    [24.182] ms
Wrote image file mandelbrot-task-ispc.ppm
                                (4.47x speedup from ISPC)
                                (8.85x speedup from task ISPC)
```
이때 실행하는 CPU는 intel의 Tiger Lake 아키텍쳐로, core 안에 2개의 8-wide vector ALU(256bit AVX2)가 들어있다. 따라서 이상적으로는 총 16배의 speedup을 기대할 수 있다. 

그러나 실제 speedup은 이것보다 한참 적은 4.47배이다. 그 이유는, 우선 `output` 배열에 접근하는 메모리 overhead가 존재하기 때문이다. 또한 `mandelbrot` 함수는 prog1에서 보듯 내부 조건문이 존재하는데, 내부 조건문에 의해 병렬성이 훼손될 수 있다. 

# part 2: ISPC Tasks
```cpp
export void mandelbrot_ispc_withtasks(uniform float x0, uniform float y0,
                                      uniform float x1, uniform float y1,
                                      uniform int width, uniform int height,
                                      uniform int maxIterations,
                                      uniform int output[])
{

    uniform int rowsPerTask = height / 2;

    // create 2 tasks
    launch[2] mandelbrot_ispc_task(x0, y0, x1, y1,
                                     width, height,
                                     rowsPerTask,
                                     maxIterations,
                                     output); 
}
```
위의 task를 나누는 코드에서 보듯 2개의 task를 실행한다. 이때 각각의 task는 아래와 같이 동작한다. 이때 task는 다른 CPU에서 병렬로 처리될 수 있다. 

따라서 이때의 이상적인 speedup은 task가 없을 때의 2배인 32배이다. 그러나 실제 speedup은 8.85배이다. 이 speedup은 task가 없을 때의 거의 2배인데, 이는 task를 나누는 것이 효과적으로 병렬 연산을 수행하고 있음을 나타낸다. 

이때 task의 개수를 늘리면 speedup이 계속 증가할 수 있을 것으로 예측할 수 있다. 실제 speedup은 아래와 같다. 
|tasks|speedup|
|:---:|:---:|
|1|4.47|
|2|8.85|
|4|9.44|
|8|15.24|
|16|19|
|32|19.31|

이때 재밌는 현상이 나타나는데, task가 4개일 때 갑자기 speedup이 꺾인다. 또한 thread 8개인 환경임에도 불구하고 task를 더 많이 할 때 speedup이 존재한다. 이는 task의 개수가 thread의 개수보다 많으면 thread context switch overhead로 인해 speedup이 제한된다는 생각과는 반대된다. 

```cpp
// slightly different kernel to support tasking
task void mandelbrot_ispc_task(uniform float x0, uniform float y0, 
                               uniform float x1, uniform float y1,
                               uniform int width, uniform int height,
                               uniform int rowsPerTask,
                               uniform int maxIterations,
                               uniform int output[])
{

    // taskIndex is an ISPC built-in
    
    uniform int ystart = taskIndex * rowsPerTask;
    uniform int yend = ystart + rowsPerTask;
    
    uniform float dx = (x1 - x0) / width;
    uniform float dy = (y1 - y0) / height;
    
    foreach (j = ystart ... yend, i = 0 ... width) {
            float x = x0 + i * dx;
            float y = y0 + j * dy;
            
            int index = j * width + i;
            output[index] = mandel(x, y, maxIterations);
    }
}
```
위에서 보듯 각각의 task는 blocking 방식으로 task를 나누어가짐을 확인할 수 있다. prog1에서 blocking 방식으로 task를 나누어가질 때 각각의 task의 computation이 크게 차이남을 확인했었다. 따라서 task가 4개일 때 각각의 task 간에 computation의 차이가 커 병렬화가 제대로 되지 않아 speedup이 작은 것이다. 

한편 task를 8개보다 크게 늘리는 경우를 생각해보면, 처음 실행하는 8개의 worker thread 중 빠르게 실행이 끝나는 worker thread에 다른 task를 mapping할 수 있다. 이런 식으로 실행이 끝나는 worker thread에 task를 mapping함으로서 각각의 worker thread의 workload balence를 맞출 수 있고, 이것이 context switch의 영향을 상회하여 speedup이 있는 것이라 추측할 수 있다. 

# thread와 task의 차이
thread는 OS가 직접 생성하고 관리하는 실행 단위이며, task는 ISPC runtime이 실행하는 작업 단위이다. 

10000개의 thread를 실행한 경우 각각의 thread를 만들기 위한 HW resource가 모자라지며 오류가 날 수 있다. 
10000개의 task를 실행한 경우 각각의 task는 존재하는 worker thread에 mapping되며, ISPC runtime이 이들을 scheduling한다. 따라서 실제 OS thread는 몇 개만 존재하므로 HW resource가 고갈되는 일은 없다. 