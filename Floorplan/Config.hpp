//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <iostream>

/// �㷨��������
struct Config {
	unsigned int random_seed; // ������ӣ�release�汾ʹ��`random_device{}();`
	double init_fill_ratio;   // ��ʼ����ʣ�����0.5
	double alpha;             // ���Ȩ��
	double beta;			  // �߳�Ȩ��
	int dimension;            // QAP������ά��. e.g.dimension=5�򻮷�25�����飬����25��block�����Ͻ�
	int ub_time;              // ASA��ʱʱ��
	int ub_iter;              // RLS����������

	enum LevelCandidateWidth {
		CombRotate, // ������ϼ���ת���������
		CombShort,  // ���Ƕ̱ߵ����
		Interval    // �Լ���Ⱦ໮�ֿ������
	} level_asa_cw; // ����Interval

	enum LevelGraphConnection {
		Direct,      // ������ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ 0
		Indirect     // ���Ƿ�ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ `1/���·����`
	} level_qapc_gc; // [todo] ��ǰ��ʵ��Direct

	enum LevelFlow {
		Kway,
		Recursive,
	} level_qapc_flow; // ����Kway

	enum LevelDistance {
		EuclideanDis,   // ŷ����þ���
		ManhattanDis,   // �����پ���
		ChebyshevDis,   // �б�ѩ�����
		EuclideanSqrDis // ŷʽƽ������
	} level_qapc_dis;   // Ĭ��ManhattanDis

	enum LevelGroupSearch {
		NoGroup,		 // ������
		Selfishly,       // ����ǰ����
		NeighborAll,     // ��ǰ�����ȫ���ھӷ���
		NeighborPartial  // [todo] ��ǰ�������/���ھӷ��飬��ɿ��ǰ��ٷֱ�ѡȡһ���־���
	} level_fbp_gs;      // Ĭ��NeighborAll

	enum LevelWireLength {
		BlockOnly,       // ������block�����߳������ڲ���[Chen.2017]���Ľ��
		BlockAndTerminal // ����block��terminal�����߳�
	} level_fbp_wl;      // Ĭ��BlockAndTerminal

	enum LevelObjNorm {
		NoNorm,         // ����һ�������ڴ��Ż����ʱ���`LevelGroupSearch::NoGroup`ʹ��
		Average,        // ʹ��ƽ��ֵ��һ��
		Minimum			// ʹ����Сֵ��һ��
	} level_fbp_norm;

	/// [deprecated] ����̰���㷨��Ч
	enum LevelHeuristicSearch {
		MinHeightFit,    // ��С�߶�rectangle�������ǿ�skyline�Ҳ����
		MinWasteFit,     // ��С�˷�rectangle�������ǿ�skyline�Ҳ����
		BottomLeftScore  // ��������skyline�����ǿ�skyline�Ҳ����
	} level_fbp_hs;      // RLS��֧��BottomLeftScore
};

std::ostream& operator<<(std::ostream &os, const Config &cfg) {
	switch (cfg.level_asa_cw) {
	case Config::LevelCandidateWidth::CombRotate: os << "CombRotate,"; break;
	case Config::LevelCandidateWidth::CombShort: os << "CombShort,"; break;
	case Config::LevelCandidateWidth::Interval: os << "Interval,"; break;
	default: break;
	}

	switch (cfg.level_qapc_gc) {
	case Config::LevelGraphConnection::Direct: os << "Direct,"; break;
	case Config::LevelGraphConnection::Indirect: os << "Indirect,"; break;
	default: break;
	}

	switch (cfg.level_qapc_flow) {
	case Config::LevelFlow::Kway: os << "Kway,"; break;
	case Config::LevelFlow::Recursive:os << "Recursive,"; break;
	default: break;
	}

	switch (cfg.level_qapc_dis) {
	case Config::LevelDistance::EuclideanDis: os << "EuclideanDis,"; break;
	case Config::LevelDistance::ChebyshevDis: os << "ChebyshevDis,"; break;
	case Config::LevelDistance::ManhattanDis: os << "ManhattanDis,"; break;
	case Config::LevelDistance::EuclideanSqrDis: os << "EuclideanSqrDis,"; break;
	default: break;
	}

	switch (cfg.level_fbp_gs) {
	case Config::LevelGroupSearch::NoGroup: os << "NoGroup,"; break;
	case Config::LevelGroupSearch::Selfishly: os << "Selfishly,"; break;
	case Config::LevelGroupSearch::NeighborAll: os << "NeighborAll,"; break;
	case Config::LevelGroupSearch::NeighborPartial: os << "NeighborPartial,"; break;
	default: break;
	}

	switch (cfg.level_fbp_wl) {
	case Config::LevelWireLength::BlockOnly: os << "BlockOnly,"; break;
	case Config::LevelWireLength::BlockAndTerminal: os << "BlockAndTerminal,"; break;
	default: break;
	}

	switch (cfg.level_fbp_norm) {
	case Config::LevelObjNorm::NoNorm: os << "NoNorm"; break;
	case Config::LevelObjNorm::Average: os << "Average"; break;
	case Config::LevelObjNorm::Minimum: os << "Minimum"; break;
	default: break;
	}

	return os;
}