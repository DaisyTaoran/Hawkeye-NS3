#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// uint32_t
#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <string.h>
#include <cstring>
#include <sys/file.h>

#include "find-root-cal.h"


namespace ns3{

void FindRootCal::PrintNodeFlow(){
	printf("fprintf all node result in mix/find_root_cal.txt\n");
    
    	FILE *fout = fopen("mix/find_root_cal.txt", "w");
    	flock(fileno(fout), LOCK_EX);
    
    	int size = vexList.size();
    	fprintf(fout, "nodeId portId flowWeight %d\n", size);
    	for(int i = 0; i < size; i++)
        	fprintf(fout, "%d %d %d\n", vexList[i].nodeIdx, vexList[i].portIdx, vexList[i].flowWeight);
        
        flock(fileno(fout), LOCK_UN);
    	fclose(fout);
    
    	return;
}

void FindRootCal::SetNextHop(std::map< Ptr<Node>, std::map< Ptr<Node>, std::vector<Ptr<Node>> > > *nexth){
	nexthop = nexth;
}


int FindRootCal::GetVertexIdx(int nodeid, int portid){
    //找Node 
    int i = 0;
    for(std::vector<VertexNode>::iterator it = vexList.begin(); it != vexList.end(); it++){
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

int FindRootCal::GetEdge(int srcvex, int dstvex){
     // 找边 
    int i = 0;
    for(std::vector<EdgeNode>::iterator it = vexList[srcvex].edges.begin(); it != vexList[srcvex].edges.end(); it++){
        if(it->dstVex == dstvex) 
            return i;
        i++;
    }
    // 没有边，于是新建一个边 
    EdgeNode e;
    e.dstVex = dstvex;
    vexList[srcvex].edges.push_back(e);

    
    return i;
}

int FindRootCal::GetEdge(int srcnode, int srcport, int dstnode, int dstport){
    // 找端点 
    int src = GetVertexIdx(srcnode, srcport); 
    int dst = GetVertexIdx(dstnode, dstport);
     // 找边 
    int i = 0;
    for(auto it = vexList[src].edges.begin(); it != vexList[src].edges.end(); it++){
        if(it->dstVex == dst) 
            return i;
        i++;
    }
    // 没有边，于是新建一个边 
    EdgeNode e;
    e.dstVex = dst;
    vexList[src].edges.push_back(e);

    
    //printf("Add Edge %d %d\t to %d %d\n",srcnode, srcport, dstnode, dstport);
    return i;
}

void FindRootCal::AddFlow(int srcvex, int dstvex, int flownum){
    	int edge = GetEdge(srcvex, dstvex);
    
	vexList[srcvex].edges[edge].flowNum += flownum;
    	vexList[dstvex].flowWeight += flownum;
    	vexList[srcvex].flowWeight -= flownum;
    	
    	printf("AddFlow: %d.%d to %d.%d %d\n", 
    		vexList[srcvex].nodeIdx, vexList[srcvex].portIdx, vexList[dstvex].nodeIdx, vexList[dstvex].portIdx, flownum);
}

void FindRootCal::AddFlow(int srcnode, int srcport, int dstnode, int dstport, int flownum){
    int edge = GetEdge(srcnode, srcport, dstnode, dstport);
    int src = GetVertexIdx(srcnode, srcport);
    int dst =  GetVertexIdx(dstnode, dstport);
    
    vexList[src].edges[edge].flowNum += flownum;
    vexList[dst].flowWeight += flownum;
    vexList[src].flowWeight -= flownum;
    
    printf("AddFlow: %d.%d to %d.%d %d\n", srcnode, srcport, dstnode, dstport, flownum);
}

int FindRootCal::CalFlowCont(int node){ // 计算telemetry_node.txt的流争用,return vexIdx------------

	printf("Add flow contention:\n");
	
	int size;
	int iport, oport;
	uint32_t flowIdx, dstIp, packetNum, pfcPausedPacketNum;
	char *line = NULL;
	size_t len = 0;

	char telemetry_path[100];
	sprintf(telemetry_path, "mix/telemetry_%d.txt", node);    
	FILE *fin = fopen(telemetry_path, "r");
	flock(fileno(fin), LOCK_SH);
	
	size = 2;
	do{
		getline(&line, &len, fin);
		if(strcmp(line,"signal\n") == 0) size--;
	}while(size > 0); // find signal part
	
	fscanf(fin, "traffic meter form port %d to port %d\n", &iport, &oport);
	
	size = 2;
	do{
		getline(&line, &len, fin);
		if(strcmp(line,"signal\n") == 0) size--;
	}while(size > 0);
	// 读取流信息
	fscanf(fin, "flow telemetry data for port %d\n", &oport);
	getline(&line, &len, fin);
	while(true){
		fscanf(fin, "%d %08x %d %d\n", &flowIdx, &dstIp, &packetNum, &pfcPausedPacketNum);
		if(dstIp == 0) break;
		if(packetNum > 0){
			AddFlow(node, iport, -1, flowIdx, packetNum);
		}
	}
	// 消磨
	size = 3;
	do{
		getline(&line, &len, fin);
		if(strcmp(line,"signal\n") == 0) size--;
	}while(size > 0);
	fscanf(fin, "flow telemetry data for port %d\n", &oport);
	getline(&line, &len, fin);
	while(true){
		fscanf(fin, "%d %08x %d %d\n", &flowIdx, &dstIp, &packetNum, &pfcPausedPacketNum);
		if(dstIp == 0) break;
	}
	
	flock(fileno(fin), LOCK_UN);
	fclose(fin);
	
	free(line);
	
	return GetVertexIdx(node, iport);
}

int FindRootCal::GetRootNode(){ // flowWeight最大的点，就是罪魁祸首
    int maxwei=0, maxi=0;
    int size = vexList.size();
    for(int i = 0; i < size; i++){
    	if(vexList[i].flowWeight > maxwei && vexList[i].nodeIdx != -1)
    		maxwei = vexList[i].flowWeight, maxi = i;
    }
    rootNodeIdx = maxi;
    return maxi; 
}


void FindRootCal::AddFlowInNode(int vexIdx){
	
	printf("\nAdd flow from Vertex %d.%d to next node:\n", vexList[vexIdx].nodeIdx, vexList[vexIdx].portIdx);
	
	int size = vexList[vexIdx].nextnode.size();
	for(int i = 0; i < size; i++){
		// get nextnode[i] and pfcPauseNum[i]
		
		// find next that vexList[next].nodeIdx = nextnode[i]
	    	int next = 0;
	    	int size = vexList.size();
	    	int nextnode = vexList[vexIdx].nextnode[i];
	    	for(next = 0; next < size; next++)
			if(vexList[next].nodeIdx == nextnode) break;
			
		// if not find: cal flow contention in telemetry_nextnode.txt	
		if(next == size){
			next = CalFlowCont(nextnode);
			printf("End flow contention\n");
		}
		// if has find: vexList[vexIdx] ---pfcPauseNum---> vexList[next]
		AddFlow(vexIdx, next, vexList[vexIdx].pfcPauseNum[i]);	
	}
	vexList[vexIdx].nextnode.clear();
	vexList[vexIdx].pfcPauseNum.clear();
	
	return;
}

void FindRootCal::ReadAllFiles(std::vector<std::string> fileNames){
	int size = fileNames.size();
	printf("Read %d files List as follow:\n", size);
	for(int i = 0; i < size; i++)
		printf("%s\n", fileNames[i].c_str());
	
	// 算出所有 flow to currNode
	for(int i = 0; i < size; i++){ 
		ReadFileForPause(fileNames[i]);	
	}
	
	// 算出所有 Node to nextNode + 流争用
	size = vexList.size();
	for(int i = 0; i < size; i++){
		if(vexList[i].nodeIdx == -1) continue;
		AddFlowInNode(i);
	}
	// 
}

void FindRootCal::ReadFileForPause(std::string &filename){
	printf("\nReadfile:%s\n", filename.c_str());
	uint32_t node, res, size;
	char* line;
	size_t len = 0;
		
	node=0;
	for(int i = 14; filename[i] != '.'; i++){
		node = (node * 10) + filename[i] - '0';
	}
		
	fin = fopen(filename.c_str(), "r");
	if(flock(fileno(fin), LOCK_SH | LOCK_NB) == -1){// 加共享锁
		perror("flock to read error");
        	fclose(fin); // 关闭文件
        	return;
	}
	
	ssize_t ret = getline(&line, &len, fin);
	if(ret == -1) printf("文件读取失败\n");
	while(ret != -1){
		if(strcmp(line, "polling\n") == 0){ //---
			ReadPolling(node);
			printf("End polling\n");
		} else if(strcmp(line, "signal\n") == 0){
			ReadSignal(node);
			printf("End signal\n");
		}
		ret = getline(&line, &len, fin);
	}
		
	flock(fileno(fin), LOCK_UN);
	fclose(fin);
	
	free(line);
	printf("End ReadFile:%s\n", filename.c_str());
	
	return;
}

int FindRootCal::GetNextHop(uint32_t node, uint32_t dstnode){
	auto i = nexthop->begin();
	while(i->first->GetId() != node){
		i++;
	};
	auto table = i->second;
	auto j = table.begin();
	while(j->first->GetId() != dstnode){
		j++;
	};
	return j->second[0]->GetId();
}

void FindRootCal::ReadPolling(uint32_t node){

	printf("ReadPolling: node %d\n", node);
	
	uint32_t port, size, dstnode;
	uint32_t flowIdx, dstIp, packetNum, pfcPausedPacketNum;
	char* line = NULL;
	size_t len = 0;
	
	do{
		getline(&line, &len, fin);
	}while(strcmp(line, "polling\n"));
	// 读取流信息
	fscanf(fin, "flow telemetry data for port %d\n", &port);
	getline(&line, &len, fin);
	while(true){
		fscanf(fin, "%d %08x %d %d\n", &flowIdx, &dstIp, &packetNum, &pfcPausedPacketNum);
		if(dstIp == 0) break;
		if(pfcPausedPacketNum > 0){
			uint32_t dstnode = (dstIp >> 8) & 0xffff;
			uint32_t nextnode = GetNextHop(node, dstnode);
			AddFlow(-1, flowIdx, node, port, pfcPausedPacketNum);
			vexList[GetVertexIdx(node, port)].pfcPauseNum.push_back(pfcPausedPacketNum);
			vexList[GetVertexIdx(node, port)].nextnode.push_back(nextnode);
					
		}
	}
	// 消磨
	size = 4;
	do{
		getline(&line, &len, fin);
		if(strcmp(line, "polling\n") == 0) size--;
	}while(size > 0);
	do{
		getline(&line, &len, fin);
		size++;
	}while(size != 3);
	
	free(line);
	
	//printf("End Flow Read in node %d\n", node);
	
	return;
}
    	
void FindRootCal::ReadSignal(uint32_t node){

	printf("ReadSignal: node %d\n", node);
	
	uint32_t port, size;
	uint32_t flowIdx, dstIp, packetNum, pfcPausedPacketNum;
	char* line = NULL;
	size_t len = 0;
	
	size = 3;
	do{
		getline(&line, &len, fin);
		if(strcmp(line, "signal\n") == 0) size--;
	}while(size > 0);
	// 读取流信息
	fscanf(fin, "flow telemetry data for port %d\n", &port);
	getline(&line, &len, fin);
	while(true){
		fscanf(fin, "%d %08x %d %d\n", &flowIdx, &dstIp, &packetNum, &pfcPausedPacketNum);
		if(dstIp == 0) break;
		if(pfcPausedPacketNum != 0){
			uint32_t dstnode = (dstIp >> 8) & 0xffff;
			uint32_t nextnode = GetNextHop(node, dstnode);
			AddFlow(-1, flowIdx, node, port, pfcPausedPacketNum);
			vexList[GetVertexIdx(node, port)].pfcPauseNum.push_back(pfcPausedPacketNum);
			vexList[GetVertexIdx(node, port)].nextnode.push_back(nextnode);
					
		}
	}
	// 消磨
	size = 3;
	do{
		getline(&line, &len, fin);
		if(strcmp(line, "signal\n") == 0) size--;
	}while(size > 0);
	fscanf(fin, "flow telemetry data for port %d\n", &port);
	getline(&line, &len, fin);
	while(true){
		fscanf(fin, "%d %08x %d %d\n", &flowIdx, &dstIp, &packetNum, &pfcPausedPacketNum);
		if(dstIp == 0) break;
	}
	
	free(line);
	
	return;
}

};

