//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

/// �㷨��������
struct Config {
	unsigned int random_seed; // ������ӣ�release�汾ʹ��`random_device{}();`
	double init_fill_ratio;   // ��ʼ����ʣ�����0.5
	double alpha;             // ���Ȩ��
	double beta;			  // �߳�Ȩ��
	int dimension;            // QAP����ά��. e.g.dimension=5�򻮷�25������
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
	} level_qapc_dis;

	enum LevelGroupSearch {
		None,		     // ������
		Selfishly,       // ����ǰ����
		NeighborAll,     // ��ǰ�����ȫ���ھӷ���
		NeighborPartial  // [todo] ��ǰ�������/���ھӷ��飬��ɿ��ǰ��ٷֱ�ѡȡһ���־���
	} level_fbp_gs;

	enum LevelWireLength {
		BlockOnly,       // ������block�����߳������ڲ���[Chen.2017]���Ľ��
		BlockAndTerminal // ����block��terminal�����߳�
	} level_fbp_wl;      // ����BlockAndTerminal

	enum LevelObjNorm {
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