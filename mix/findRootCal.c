#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

struct EdgeNode {// 边表结点
        int dstVex;         // 表示该边的终点
        int flowNum;
        EdgeNode(){
            flowNum=0; 
        }
};

struct VertexNode {// 顶点  
        int nodeIdx, portIdx; // nodeIdx=-1代表此节点代表某个流，而非某Node，此时portIdx就是流Idx
        int inDegree, outDegree;
        int flowWeight;
        vector<EdgeNode> edges; // 记录从这个点出发的所有边 
        VertexNode(){
            inDegree = 0; outDegree = 0;
            flowWeight = 0; 
        } 
};   

// 可以识别根本原因（例如流争用）、PFC传播路径和受害流。
class FindRootCal{    
public:
    FindRootCal(){}
    void PrintNodeFlow();
    void AddFlow(int srcnode, int srcport, int dstnode, int dstport, int flownum);
    void ShowRootNode();//--------------
    
private:
    vector<VertexNode> vexList; 
    
    int GetVertexIdx(int nodeid, int portid);
    int GetEdge(int srcnode, int srcport, int dstnode, int dstport);
};

int main(){
    FindRootCal g;
    g.AddFlow(-1,764,9,1,26);
    g.AddFlow(-1,760,9,1,5);
    g.AddFlow(9,1,10,3,31);
    g.AddFlow(-1,760,10,3,22);
    g.AddFlow(10,3,12,1,22);
    g.AddFlow(12,1,-1,760,95);
    g.AddFlow(12,1,-1,705,1530);
    g.AddFlow(12,1,-1,806,1530);
    g.AddFlow(12,1,-1,876,1530);
    g.AddFlow(12,1,-1,989,1530);
    g.PrintNodeFlow();
    return 0;
} 


int FindRootCal::GetVertexIdx(int nodeid, int portid){
    //找Node 
    int i = 0;
    for(vector<VertexNode>::iterator it = vexList.begin(); it != vexList.end(); it++){
        if(it->nodeIdx == nodeid && it->portIdx == portid) 
            return i;        
        i++;
    }
    // 若没有Node，就新建一个 
    VertexNode v;
    v.nodeIdx = nodeid; v.portIdx = portid;
    vexList.push_back(v);
    //printf("Add Vertex %d %d\n", nodeid, portid);
    
    return i;
}

int FindRootCal::GetEdge(int srcnode, int srcport, int dstnode, int dstport){
    // 找端点 
    int src = GetVertexIdx(srcnode, srcport); 
    int dst = GetVertexIdx(dstnode, dstport);
     // 找边 
    int i = 0;
    for(vector<EdgeNode>::iterator it = vexList[src].edges.begin(); it != vexList[src].edges.end(); it++){
        if(it->dstVex == dst) 
            return i;
        i++;
    }
    // 没有边，于是新建一个边 
    EdgeNode e;
    e.dstVex = dst;
    vexList[src].edges.push_back(e);

    vexList[src].outDegree++;
    vexList[dst].inDegree++;
    
    printf("Add Edge %d %d\t to %d %d\n",srcnode, srcport, dstnode, dstport);
    return i;
}

void FindRootCal::AddFlow(int srcnode, int srcport, int dstnode, int dstport, int flownum){
    int edge = GetEdge(srcnode, srcport, dstnode, dstport);
    int src = GetVertexIdx(srcnode, srcport);
    int dst =  GetVertexIdx(dstnode, dstport);
    
    vexList[src].edges[edge].flowNum += flownum;
    vexList[dst].flowWeight += flownum;
    vexList[src].flowWeight -= flownum;
}

void FindRootCal::PrintNodeFlow(){
    printf("\nnodeId\t portId\t flowWeight\n");
    int size = vexList.size();
    for(int i = 0; i < size; i++)
        printf("%d\t  %d\t  %d\n", vexList[i].nodeIdx, vexList[i].portIdx, vexList[i].flowWeight);
}

void FindRootCal::ShowRootNode(){
    // flowWeight最大的点，就是罪魁祸首
     
}

