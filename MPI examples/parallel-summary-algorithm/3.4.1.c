#include <stdio.h>
#include <string.h>
#include <mpi.h>

/*习题3.4的第一部分：使用蝶形结构(p71 图3-9)，且处理进程数量comm_sz为2的幂的情况*/
/*当进程数量comm_sz是2的幂时，假设进程数量为comm_sz，假设数组的总大小为total_sz*/
/*1.首先将数组中的total_size个元素分配到comm_sz个进程中，具体而言假设step=floor(total_sz/comm_sz)，则进程编号为0到comm_sz-2的进程每个处理的元素数量为step，具体而言，进程i处理的下标范围为[i*step,(i+1)*step)；最后一个进程处理的元素个数为total_sz-(comm_sz-1)*floor(total_sz/comm_sz)，其对应处理的范围为[i*step,total_sz)*/
/*2.然后，各个进程对它们所负责的数组元素范围进行串行求和*/
/*3.然后进行进程通信，我们使用蝶形结构，假设步长变量为step=1*/
/*4.首先，每个进程仅仅只知道当前的进程号my_rank，在蝶形进程中，全部的进程一致都是活跃进程，且在每一次迭代中，每个进程既是接收方也是发送方，因此只需要确认当前进程从哪个进程接收消息，并且将消息发送给哪个进程即可
    (1)首先按照步长将各个进程分成comm_sz/step组，且对于当前进程i，查找并确认当前进程i属于哪一组，具体而言，若当前进程的编号在区间[k*step,(k+1)*step]，则当前进程属于第k组
    (2)假设当前进程为第k组，则若当前进程属于第k组，若k为奇数，则该进程首先向进程(my_rank-step)发送自己的节点值，然后再从(my_rank-step)接收信息，最后再将接收到的值和自己的节点值相加
    (3)若k为偶数，则该进程首先从进程(my_rank+step)接收信息，然后再向节点(my_rank+step)发送自己的节点值，最后再将接收到的值和自己的节点值相加*/
/*5.将步长加倍，即step=step*2，重复上述步骤4，直到step=comm_sz时，退出循环*/
int main(void)
{
    int comm_sz=8;         /*本并行程序所使用的进程数量（变量）*/
    int my_rank;           /*当前进程的编号*/
    double lis[]={5,2,-1,-3,6,5,-7,2};   /*待求和的数组*/    
    int total_sz=8;        /*数组总大小*/

    /*创建MPI进程并获取当前的进程编号*/
    MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);     /*指定进程数量*/
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);     /*获取进程编号*/
    
    /*每一个进程首先先计算出需要计算的那一部分数组的范围*/
    int stepa=total_sz/comm_sz;
    int begin=my_rank*stepa;
    int end=(my_rank==comm_sz-1)?total_sz:(my_rank+1)*stepa;
    int i=0;
    double add=0;       /*当前进程所负责的数组范围内的元素求和结果*/
    for(i=begin;i<end;i++)
    {
        add+=lis[i];
    }
    
    /*进行进程通信*/
    /*此处执行进程通信时，每个进程初始时仅仅已知进程总数和自己的进程编号，这与串行编程有很大不同，仅能站在进程的角度，仅仅根据这两个已知量来设计通信过程，而不能站在上帝视角*/
    int step=1;             /*步长，步长的初始值为1*/
    double receive_res=0;   /*从其他进程接收到的结果，初始值必须设为0！*/
    while(step!=comm_sz)
    {
        /*首先根据步长step进行分组，确定当前进程属于第几组*/ 
        int k;
        for(k=0;k<comm_sz/step;k++)
        {
            if(my_rank>=k*step&&my_rank<(k+1)*step)   /*这里也可以直接使用模运算%实现，由于实现的原理一致，此处略去具体方法*/
            break;
        }
        /*此时进程的组数就是k，则按照k是奇数偶数进行不同的通信操作*/
        if(k%2)   /*若k为奇数，则该进程首先向进程(my_rank-step)发送自己的节点值，然后再从(my_rank-step)接收信息，最后再将接收到的值和自己的节点值相加*/
        {
            MPI_Send(&add,1,MPI_DOUBLE,my_rank-step,0,MPI_COMM_WORLD);
            MPI_Recv(&receive_res,1,MPI_DOUBLE,my_rank-step,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            add+=receive_res;
            receive_res=0;     /*重置从其他进程接收到的结果*/
        }
        else      /*若k为偶数，则该进程首先从进程(my_rank+step)接收信息，然后再向节点(my_rank+step)发送自己的节点值，最后再将接收到的值和自己的节点值相加*/
        {
            MPI_Recv(&receive_res,1,MPI_DOUBLE,my_rank+step,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(&add,1,MPI_DOUBLE,my_rank+step,0,MPI_COMM_WORLD);
            add+=receive_res;
            receive_res=0;     /*重置从其他进程接收到的结果*/
        }
        step*=2;
    }

    /*输出运行结果*/
    /*由进程0输出运行结果*/
    if(my_rank==0)
    {
        printf("%lf",add);
    }
    MPI_Finalize();
}