//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <metis.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

namespace metis {

	using namespace std;

	vector<idx_t> func(vector<idx_t>& xadj, vector<idx_t>& adjncy, vector<idx_t>& adjwgt, decltype(METIS_PartGraphKway)* METIS_PartGraphFunc) {
		idx_t nVertices = xadj.size() - 1; // �ڵ���
		idx_t nEdges = adjncy.size() / 2;  // ����
		idx_t nWeights = 1;                // �ڵ�Ȩ��ά��
		idx_t nParts = 2;                  // ��ͼ������2
		idx_t objval;                      // Ŀ�꺯��ֵ
		vector<idx_t> part(nVertices, 0);  // ���ֽ��

		int ret = METIS_PartGraphFunc(&nVertices, &nWeights, xadj.data(), adjncy.data(),
			NULL, NULL, adjwgt.data(), &nParts, NULL,
			NULL, NULL, &objval, part.data());

		if (ret != rstatus_et::METIS_OK) { cout << "METIS_ERROR" << endl; }
		cout << "METIS_OK" << endl;
		cout << "objval: " << objval << endl;
		for (unsigned part_i = 0; part_i < part.size(); part_i++) {
			cout << part_i + 1 << " " << part[part_i] << endl;
		}

		return part;
	}

	void test_metis() {
		ifstream ingraph("../Lib/metis/case/graph_b.txt");

		int vexnum, edgenum;
		string line;
		getline(ingraph, line);
		istringstream tmp(line);
		tmp >> vexnum >> edgenum;
		vector<idx_t> xadj(0);
		vector<idx_t> adjncy(0); // ѹ��ͼ��ʾ
		vector<idx_t> adjwgt(0); // �ڵ�Ȩ��

		idx_t a, w;
		for (int i = 0; i < vexnum; i++) {
			xadj.push_back(adjncy.size());
			getline(ingraph, line);
			istringstream tmp(line);
			while (tmp >> a >> w) {
				adjncy.push_back(a - 1); // �ڵ�id��0��ʼ
				adjwgt.push_back(w);
			}
		}
		xadj.push_back(adjncy.size());
		ingraph.close();

		vector<idx_t> part = func(xadj, adjncy, adjwgt, METIS_PartGraphRecursive);
		//vector<idx_t> part = func(xadj, adjncy, adjwgt, METIS_PartGraphKway);

		ofstream outpartition("../Lib/metis/case/partition_b.txt");
		for (int i = 0; i < part.size(); i++) { outpartition << i + 1 << " " << part[i] << endl; }
		outpartition.close();
	}
}
