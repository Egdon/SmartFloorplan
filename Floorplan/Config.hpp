//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#ifdef NDEBUG
#define debug_run(x)
#else
#define debug_run(x) x
#endif // !NDEBUG

#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <random>

static constexpr int INF = 0x3f3f3f3f;

static std::vector<std::pair<std::string, std::string>> ins_list{
	{"MCNC", "apte"},{"MCNC", "hp"},{"MCNC", "xerox"},{"MCNC", "ami33"},{"MCNC", "ami49"},
	{"GSRC", "n10"},{"GSRC", "n30"},{"GSRC", "n50"},{"GSRC", "n100"},{"GSRC", "n200"},{"GSRC", "n300"}
};

/// �㷨��������
struct Config {
	unsigned int random_seed = std::random_device{}(); // ������ӣ�release�汾ʹ��`random_device{}();`
	//unsigned int random_seed = 2231403657;

	double alpha = 1.0, beta = 0.0;        // ����������߳��Ż�Ȩ��
	double lb_scale = 0.8, ub_scale = 1.2; // ���Ʒ�֧��Ŀ�������

	int ub_time = 3600; // ASA��ʱʱ��
	int ub_iter = 9999; // RLS����������/BS��������

	int dimension = 5;  // QAP������ά��. e.g.dimension=5�򻮷�25�����飬����25��block�����Ͻ�

	enum LevelCandidateWidth { // [deprecated]
		CombRotate,            // ������ϼ���ת���������
		CombShort,             // ���Ƕ̱ߵ����
		Interval               // �Լ���Ⱦ໮�ֿ������
	} level_asa_cw = Interval;

	enum LevelFloorplanPacker {
		RandomLocalSearch,
		BeamSearch
	} level_asa_fbp = BeamSearch;

	enum LevelWireLength {
		BlockOnly,         // ����block�ڲ������߳�
		BlockAndTerminal   // [deprecated] ����block��terminal�����߳�
	} level_fbp_wl = BlockOnly;

	enum LevelObjective { // [deprecated]
		AvgNorm,          // ʹ��ƽ��ֵ��һ��
		MinNorm,	      // ʹ����Сֵ��һ��
		EDAthon           // ʹ��EDAthon2020P4Ŀ�꺯��
	} level_fbp_obj = EDAthon;

	enum LevelQAPCluster {
		On,
		Off // ��ʹ��QAP
	} level_rls_qapc = Off;

	enum LevelGroupSearch {
		NoGroup,		 // �����飬LevelQAPCluster::Off <==> LevelGroupSearch::NoGroup
		NeighborNone,    // ��ǰ����
		NeighborAll,     // ��ǰ�����ȫ���ھӷ���
		NeighborPartial  // ��ǰ����������ھӷ��飬[todo]���ǰ��ٷֱ�ѡȡһ���־���?
	} level_rls_gs = NoGroup;

	enum LevelGraphConnect {
		Direct,  // ������ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ 0
		Indirect // ���Ƿ�ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ `1/���·����`
	} level_qapc_gc = Direct;

	enum LevelFlow {
		Kway,
		Recursive,
	} level_qapc_flow = Kway;

	enum LevelDist {
		EuclideanDist,   // ŷ����þ���
		ManhattanDist,   // �����پ���
		ChebyshevDist,   // �б�ѩ�����
		EuclideanSqrDist // ŷʽƽ������
	} level_qapc_dis = ManhattanDist;
} cfg;

std::ostream& operator<<(std::ostream &os, const Config &cfg) {
	switch (cfg.level_asa_fbp) {
	case Config::LevelFloorplanPacker::RandomLocalSearch: os << "RandomLocalSearch,"; break;
	case Config::LevelFloorplanPacker::BeamSearch: os << "BeamSearch,"; break;
	default: break;
	}

	switch (cfg.level_fbp_wl) {
	case Config::LevelWireLength::BlockOnly: os << "BlockOnly,"; break;
	case Config::LevelWireLength::BlockAndTerminal: os << "BlockAndTerminal,"; break;
	default: break;
	}

	switch (cfg.level_rls_qapc) {
	case Config::LevelQAPCluster::On: os << "On,"; break;
	case Config::LevelQAPCluster::Off: os << "Off,"; break;
	default: break;
	}

	switch (cfg.level_rls_gs) {
	case Config::LevelGroupSearch::NoGroup: os << "NoGroup,"; break;
	case Config::LevelGroupSearch::NeighborNone: os << "NeighborNone,"; break;
	case Config::LevelGroupSearch::NeighborAll: os << "NeighborAll,"; break;
	case Config::LevelGroupSearch::NeighborPartial: os << "NeighborPartial,"; break;
	default: break;
	}

	switch (cfg.level_qapc_gc) {
	case Config::LevelGraphConnect::Direct: os << "Direct,"; break;
	case Config::LevelGraphConnect::Indirect: os << "Indirect,"; break;
	default: break;
	}

	switch (cfg.level_qapc_flow) {
	case Config::LevelFlow::Kway: os << "Kway,"; break;
	case Config::LevelFlow::Recursive:os << "Recursive,"; break;
	default: break;
	}

	switch (cfg.level_qapc_dis) {
	case Config::LevelDist::EuclideanDist: os << "EuclideanDist"; break;
	case Config::LevelDist::ChebyshevDist: os << "ChebyshevDist"; break;
	case Config::LevelDist::ManhattanDist: os << "ManhattanDist"; break;
	case Config::LevelDist::EuclideanSqrDist: os << "EuclideanSqrDist"; break;
	default: break;
	}

	return os;
}
