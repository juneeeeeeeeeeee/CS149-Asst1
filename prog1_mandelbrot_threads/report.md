[Prog 1]
1.	2개의 thread를 사용해 공간 분해
다음과 같이 코드를 수정했다. 이때 thread 개수 확장성을 위해 numThreads 변수를 사용하였다. 한편 thread 개수가 7개인 경우 remainder가 있어 일부 행이 실행되지 않는 문제가 발생하여 remainder를 더해주기로 하였고, 이를 위해 각 thread의 argument를 저장하는 structure에 두 개의 변수(startRow, numRows)를 추가하였다. 
 
 
2개의 thread를 사용했을 때 1.82배의 speedup이 있었다. 
 
2.	Block을 이용한 2-8개 thread로 확장
여러 개의 thread를 사용했을 때 speedup은 아래의 그래프와 같다. 
 
현재 사용중인 system은 4개의 core를 가지며, 하나의 core당 2개의 thread를 사용한다. 이때 speedup이 linear하지 않고, 쓰레드가 3개일 때 speedup이 감소함을 확인할 수 있다. 
한편 view 2의 경우 main code에서



