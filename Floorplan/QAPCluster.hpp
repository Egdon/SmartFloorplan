//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <cmath>
#include <metis.h>
#include <gurobi_c++.h>
#include "../QAPSolver/QAPSolver.hpp"

#include "Instance.hpp"

namespace qapc {

	/// ������Ԥ�����QAP����
	class QAPCluster {

	public:
		enum LevelConnection {
			Direct,    // ������ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ 0
			Indirect   // ���Ƿ�ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ `1/���·����`
		};

		enum LevelMetis {
			Kway,
			Recursive,
		};

		enum LevelDistance {
			EuclideanDis,   // ŷ����þ���
			ManhattanDis,   // �����پ���
			ChebyshevDis,   // �б�ѩ�����
			EuclideanSqrDis // ŷʽƽ������
		};

		QAPCluster(const Instance &ins, int dimension) : _ins(ins), _dimension(dimension) {
			build_graph(LevelConnection::Direct);
		}

		/// ����QAP��flow matrix
		/// [deprecated] gurobi_cluster(flow_matrix, _dimension * _dimension, bin_area);
		vector<vector<int>> cal_flow_matrix(LevelMetis method) {
			vector<vector<int>> flow_matrix;
			switch (method) {
			case LevelMetis::Kway:
				metis_cluster(flow_matrix, _dimension * _dimension, METIS_PartGraphKway);
				break;
			case LevelMetis::Recursive:
				metis_cluster(flow_matrix, _dimension * _dimension, METIS_PartGraphRecursive);
				break;
			default:
				assert(false);
				break;
			}
			return flow_matrix;
		}

		/// ����QAP��distance matrix
		vector<vector<int>> cal_distance_matrix(LevelDistance method) {
			int node_num = _dimension * _dimension;
			_distance_nodes.resize(node_num);
			for (int x = 0; x < _dimension; ++x) {
				for (int y = 0; y < _dimension; ++y) {
					_distance_nodes[x * _dimension + y].first = x;
					_distance_nodes[x * _dimension + y].second = y;
				}
			}
			vector<vector<int>> distance_matrix(node_num, vector<int>(node_num, 0));
			for (int i = 0; i < node_num; ++i) {
				for (int j = i + 1; j < node_num; ++j) {
					distance_matrix[i][j] = cal_distance(method,
						_distance_nodes[i].first, _distance_nodes[i].second,
						_distance_nodes[j].first, _distance_nodes[j].second);
					distance_matrix[j][i] = distance_matrix[i][j];
				}
			}
			return distance_matrix;
		}

		/// ����qap����
		void cal_qap_sol(const vector<vector<int>> &flow_matrix, const vector<vector<int>> &distance_matrix) {
			qap::run_qap(flow_matrix, distance_matrix, qap_sol);
		}

		/// ��������ھ���Ϣ
		vector<vector<bool>> cal_group_neighbors() const {
			int group_num = _dimension * _dimension;
			vector<vector<bool>> group_neighbors(group_num, vector<bool>(group_num, false));
			for (int gi = 0; gi < group_num; ++gi) {
				for (int gj = gi + 1; gj < group_num; ++gj) {
					if (1 == cal_distance(LevelDistance::ChebyshevDis, // �б�ѩ�����Ϊ1����Ϊ�ھ�
						_distance_nodes.at(gi).first, _distance_nodes.at(gi).second,
						_distance_nodes.at(gj).first, _distance_nodes.at(gj).second)) {
						group_neighbors[gi][gj] = true;
						group_neighbors[gj][gi] = true;
					}
				}
			}
			return group_neighbors;
		}

		/// �������߽���Ϣ
		vector<rbp::Boundary> cal_group_boundaries(int bin_width, int bin_height) const {
			int group_num = _dimension * _dimension;
			vector<rbp::Boundary> group_boundaries(group_num);
			double unit_width = 1.0*bin_width / _dimension;
			double unit_height = 1.0*bin_height / _dimension;
			for (int gi = 0; gi < group_num; ++gi) {
				group_boundaries[gi].x = unit_width * _distance_nodes.at(gi).first;
				group_boundaries[gi].y = unit_height * _distance_nodes.at(gi).second;
				group_boundaries[gi].width = unit_width;
				group_boundaries[gi].height = unit_height;
			}
			return group_boundaries;
		}

		static int cal_distance(LevelDistance method, int x1, int y1, int x2, int y2) {
			int distance = 0;
			switch (method) {
			case LevelDistance::EuclideanDis:
				distance = round(sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2)));
				break;
			case LevelDistance::ManhattanDis:
				distance = abs(x1 - x2) + abs(y1 - y2);
				break;
			case LevelDistance::ChebyshevDis:
				distance = max(abs(x1 - x2), abs(y1 - y2));
				break;
			case LevelDistance::EuclideanSqrDis:
				distance = pow(x1 - x2, 2) + pow(y1 - y2, 2);
				break;
			default:
				assert(false);
				break;
			}
			return distance;
		}

	private:
		/// ���net_list��ԭ��ͼ
		/// [todo] ��Ҫ��terminalҲ����ͼ��
		/// [todo] ���Ƿ�ֱ���ıߣ���Ҫ������·���㷨����������metis��֧�ָ�����Ȩ��
		void build_graph(LevelConnection method) {
			_graph.resize(_ins.get_block_num(), vector<int>(_ins.get_block_num(), 0));
			for (auto &net : _ins.get_net_list()) {
				for (int i = 0; i < net.block_list.size(); ++i) {
					for (int j = i + 1; j < net.block_list.size(); ++j) {
						int a = net.block_list[i];
						int b = net.block_list[j];
						_graph[a][b] += 1;
						_graph[b][a] += 1;
					}
				}
			}

			if (method == LevelConnection::Indirect) {}
		}

		/// �ڽӾ���תѹ��ͼ��CSR��
		void transform_graph_2_csr(vector<idx_t> &xadj, vector<idx_t> &adjncy, vector<idx_t> &vwgt, vector<idx_t> &adjwgt) const {
			xadj.reserve(_graph.size() + 1);
			vwgt.reserve(_graph.size());
			adjncy.reserve(_graph.size() * _graph.size());
			adjwgt.reserve(_graph.size() * _graph.size());
			for (int i = 0; i < _graph.size(); ++i) {
				xadj.push_back(adjncy.size());
				vwgt.push_back(_ins.get_block_area(i));
				for (int j = 0; j < _graph.size(); ++j) {
					if (i == j) { continue; }
					if (_graph[i][j] >= 1) {
						adjncy.push_back(j);
						adjwgt.push_back(_graph[i][j]);
					}
				}
			}
			xadj.push_back(adjncy.size());
		}

		/// metis����������
		void metis_cluster(vector<vector<int>> &flow_matrix, int group_num, decltype(METIS_PartGraphKway) *METIS_PartGraphFunc) {
			flow_matrix.clear();
			flow_matrix.resize(group_num, vector<int>(group_num, 0));

			vector<idx_t> xadj, adjncy, vwgt, adjwgt;
			transform_graph_2_csr(xadj, adjncy, vwgt, adjwgt);
			idx_t nvtxs = xadj.size() - 1;
			idx_t ncon = 1;
			idx_t nparts = group_num;
			idx_t objval;
			part.resize(nvtxs, 0);
			int ret = METIS_PartGraphFunc(&nvtxs, &ncon, xadj.data(), adjncy.data(),
				vwgt.data(), NULL, adjwgt.data(), &nparts, NULL,
				NULL, NULL, &objval, part.data());
			assert(ret == rstatus_et::METIS_OK);

			for (unsigned part_i = 0; part_i < part.size(); ++part_i) {
				for (unsigned part_j = part_i + 1; part_j < part.size(); ++part_j) {
					if (part[part_i] == part[part_j]) { continue; }
					flow_matrix[part[part_i]][part[part_j]] += _graph[part_i][part_j];
					flow_matrix[part[part_j]][part[part_i]] += _graph[part_j][part_i];
				}
			}
		}

		/// [deprecated] ʹ����ѧģ������������
		void gurobi_cluster(vector<vector<int>> &flow_matrix, int group_num, int bin_area) {
			flow_matrix.clear();
			flow_matrix.resize(group_num, vector<int>(group_num, 0));

			try {
				// Initialize environment & empty model
				GRBEnv env = GRBEnv(true);
				env.set("LogFile", "qap_cluster.log");  // [todo] ����ر�cmd���
				env.start();
				GRBModel gm = GRBModel(env);
				//gm.set(GRB_DoubleParam_TimeLimit, 60.0 * 10);

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
							gm.addConstr(expr >= 0);
							//gm.addConstr(expr <= 1); // ���٣����Ŀ�����ɾ������Լ��
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
								// [todo] ��������flow_matrix
								std::cout << i << " is in group " << p << std::endl;
							}
						}
					}
					std::cout << "Obj: " << gm.get(GRB_DoubleAttr_ObjVal) << std::endl;
				}
				else if (status == GRB_INFEASIBLE) {
					std::cout << "The model is infeasible; computing IIS..." << std::endl;
					gm.computeIIS();
					gm.write("cluster_model.ilp");
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

	public:
		// `qap_sol[part[rect.id]] = distance_node_id = group_id`
		vector<int> part;
		vector<int> qap_sol;

	private:
		const Instance &_ins;
		const int _dimension;
		vector<vector<int>> _graph;
		vector<pair<int, int>> _distance_nodes;
	};

}