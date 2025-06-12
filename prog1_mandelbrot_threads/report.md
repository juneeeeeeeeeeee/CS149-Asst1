[Prog 1]
1.	2개의 thread를 사용해 공간 분해
다음과 같이 코드를 수정했다. 이때 thread 개수 확장성을 위해 numThreads 변수를 사용하였다. 한편 thread 개수가 7개인 경우 remainder가 있어 일부 행이 실행되지 않는 문제가 발생하여 remainder를 더해주기로 하였고, 이를 위해 각 thread의 argument를 저장하는 structure에 두 개의 변수(startRow, numRows)를 추가하였다. 
```cpp
void workerThreadStart(WorkerArgs * const args) {

    mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width, args->height, args->startRow, args->numRows, args->maxIterations, args->output);
    // printf("Hello world from thread %d\n", args->threadId);
}
```
```cpp
for (int i=0; i<numThreads; i++) {
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].startRow = i * (height / numThreads);
        args[i].numRows = height / numThreads;
        if (i == numThreads - 1) args[i].numRows += height % numThreads; // last rows
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;
        
        args[i].threadId = i;
    }
```
```
[mandelbrot thread] : [275.387] ms
Wrote image file mandelbrot—thread.ppm (1.82x speedup frm 2 threads)
```
2개의 thread를 사용했을 때 1.82배의 speedup이 있었다. 

2.	Block을 이용한 2-8개 thread로 확장
여러 개의 thread를 사용했을 때 speedup은 아래의 그래프와 같다. 
![img](./speedup.png) 
현재 사용중인 system은 4개의 core를 가지며, 하나의 core당 2개의 thread를 사용한다. 이때 speedup이 linear하지 않고, 쓰레드가 3개일 때 speedup이 감소함을 확인할 수 있다. 
한편 view 2의 경우 main code에서



