//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <gurobi_c++.h>
#include "Instance.hpp"


/// ������Ԥ�����QAP����
class QAPCluster {
public:
	QAPCluster(const Instance &ins) : _ins(ins) { build_graph(LevelConnection::LevelDirect); }

	/// ʹ����ѧģ��������, P��������Ŀ
	void gurobi_cluster(int group_num, int bin_area) {
		try {
			// Initialize environment & empty model
			GRBEnv env = GRBEnv(true);
			//env.set("LogFile", "qap_cluster.log");
			env.start();
			GRBModel gm = GRBModel(env);
			gm.set(GRB_DoubleParam_TimeLimit, 60.0);

			// Decision Variables
			// x_ip:  block_i�Ƿ񱻷ֵ�group_p.
			// y_ijp: edge_ij�Ƿ񱻷ֵ�group_p.
			vector<vector<GRBVar>> x(_graph.size(), vector<GRBVar>(group_num));
			for (int i = 0; i < x.size(); ++i) {
				for (int p = 0; p < x[i].size(); ++p) {
					x[i][p] = gm.addVar(0, 1, 0, GRB_BINARY);
				}
			}
			vector<vector<vector<GRBVar>>> y(_graph.size(), vector<vector<GRBVar>>(_graph.size(), vector<GRBVar>(group_num)));
			for (int i = 0; i < y.size(); ++i) {
				for (int j = 0; j < y[i].size(); ++j) {
					for (int p = 0; p < y[i][j].size(); ++p) {
						y[i][j][p] = gm.addVar(0, 1, 0, GRB_BINARY);
					}
				}
			}

			// Constraint
			// Sum(x_ip) = 1: block_iһ��Ҫ���䵽ĳһ��group_p��.
			// y_ijp = x_ip �� x_jp: block_i��block_j�����䵽group_p�У���edge_ijҲ�����䵽p��.
			// Sum(x_ip * area_i) <= bin_area/group_num: group_p�е�block���Լ��.
			// [todo] ���Լ������ת��Ϊ˫Ŀ���Ż���min(����������)������p-center.
			// [todo] bin_area��Ϊ����������ѿ��ƣ�ת��Ϊ˫Ŀ���Ż��󽫲�����Ҫ��bin_area.
			// [todo] gurobiҪ����Ŀ�꺯�����������ͬsense�����Ż�����ͬΪ���С��.
			for (int i = 0; i < _graph.size(); ++i) {
				GRBLinExpr sum_xip = 0;
				for (int p = 0; p < group_num; ++p) {
					sum_xip += x[i][p];
				}
				gm.addConstr(sum_xip, GRB_EQUAL, 1);
			}
			for (int p = 0; p < group_num; ++p) {
				for (int i = 0; i < _graph.size(); ++i) {
					for (int j = 0; j < _graph.size(); ++j) {
						if (i == j) { continue; }
						GRBLinExpr expr = x[i][p] + x[j][p] - 2 * y[i][j][p];
						gm.addConstr(expr >= 0); // ���Ŀ����Խ���������Լ��
						gm.addConstr(expr <= 1);
					}
				}
			}
			for (int p = 0; p < group_num; ++p) {
				GRBLinExpr sum_area_p = 0;
				for (int i = 0; i < _graph.size(); ++i) {
					sum_area_p += x[i][p] * _ins.get_block_area(i);
				}
				gm.addConstr(sum_area_p <= bin_area / group_num);
			}

			// Objective Function
			// maximize Sum(y_ijp * w_ij): ���p�������Ȩ��֮��.
			GRBLinExpr obj = 0;
			for (int p = 0; p < group_num; ++p) {
				for (int i = 0; i < _graph.size(); ++i) {
					for (int j = 0; j < _graph.size(); ++j) {
						if (i == j) { continue; }
						obj += y[i][j][p] * _graph[i][j];
					}
				}
			}
			gm.setObjective(obj, GRB_MAXIMIZE);

			// Optimize model
			gm.optimize();
			int status = gm.get(GRB_IntAttr_Status);
			if (status == GRB_OPTIMAL || status == GRB_TIME_LIMIT) {
				for (int i = 0; i < x.size(); ++i) {
					for (int p = 0; p < x[i].size(); ++p) {
						if (x[i][p].get(GRB_DoubleAttr_X)) {
							std::cout << i << " is in group " << p << std::endl;
						}
					}
				}
				std::cout << "Obj: " << gm.get(GRB_DoubleAttr_ObjVal) << std::endl;
			}
			else if (status == GRB_INFEASIBLE) {
				std::cout << "The model is infeasible; computing IIS..." << std::endl;
				gm.computeIIS();
				gm.write("model.ilp");
			}
		}
		catch (GRBException &e) {
			std::cout << "Error code = " << e.getErrorCode() << std::endl;
			std::cout << e.getMessage() << std::endl;
			return;
		}
		catch (...) {
			std::cout << "Exception during optimization." << std::endl;
			return;
		}
	}

private:
	enum LevelConnection {
		LevelDirect,    // ������ֱ���ıߣ���ֱ���ı�Ȩ��Ϊ0
		LevelIndirect   // ���Ƿ�ֱ���ıߣ���ֱ���ı�Ȩ��Ϊ `1/���·����`
	};

	/// ���net_list��ԭ��ͼ
	void build_graph(LevelConnection method) {
		int node_num = _ins.get_block_num();  // [todo] ��Ҫ��terminalҲ����ͼ��
		_graph.resize(node_num, vector<int>(node_num, 0));
		auto &net_list = _ins.get_net_list();
		for (auto &net : net_list) {
			for (int i = 0; i < net.block_list.size(); ++i) {
				for (int j = i + 1; j < net.block_list.size(); ++j) {
					int a = net.block_list[i];
					int b = net.block_list[j];
					_graph[a][b] += 1;
					_graph[b][a] += 1;
				}
			}
		}

		// [todo] ���Ƿ�ֱ���ıߣ���Ҫ������·���㷨������
		if (method == LevelConnection::LevelIndirect) {}
	}

private:
	const Instance &_ins;
	vector<vector<int>> _graph;
};
