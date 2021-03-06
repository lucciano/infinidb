/* Copyright (C) 2013 Calpont Corp.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation;
   version 2.1 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA. */

//  $Id: tupleaggregatestep.h 9732 2013-08-02 15:56:15Z pleblanc $


#ifndef JOBLIST_TUPLEAGGREGATESTEP_H
#define JOBLIST_TUPLEAGGREGATESTEP_H

#include "jobstep.h"
#include "rowaggregation.h"

#include <boost/thread.hpp>


namespace joblist
{

// forward reference
struct JobInfo;

/** @brief class TupleAggregateStep
 *
 */
class TupleAggregateStep : public JobStep, public TupleDeliveryStep
{
public:
	/** @brief TupleAggregateStep constructor
	 */
	TupleAggregateStep(
			const rowgroup::SP_ROWAGG_UM_t&,
			const rowgroup::RowGroup&,
			const rowgroup::RowGroup&,
			const JobInfo&);

	/** @brief TupleAggregateStep destructor
	 */
   ~TupleAggregateStep();

	/** @brief virtual void Run method
	 */
	void run();
	void join();

	const std::string toString() const;

	void setOutputRowGroup(const rowgroup::RowGroup&);
	const rowgroup::RowGroup& getOutputRowGroup() const;
	const rowgroup::RowGroup& getDeliveredRowGroup() const;
	void  deliverStringTableRowGroup(bool b);
	bool  deliverStringTableRowGroup() const;
	uint nextBand(messageqcpp::ByteStream &bs);
	uint nextBand_singleThread(messageqcpp::ByteStream &bs);
	bool setPmHJAggregation(JobStep* step);
	void savePmHJData(rowgroup::SP_ROWAGG_t&, rowgroup::SP_ROWAGG_t&, rowgroup::RowGroup&);

	bool umOnly() const { return fUmOnly; }
	void umOnly(bool b) { fUmOnly = b; }

	void configDeliveredRowGroup(const JobInfo&);
	//void setEidMap(std::map<int, int>& m) { fIndexEidMap = m; }

	static SJSTEP prepAggregate(SJSTEP&, JobInfo&);

	// for multi-thread variables
	void initializeMultiThread();

private:
	static void prep1PhaseDistinctAggregate(
		JobInfo&, std::vector<rowgroup::RowGroup>&, std::vector<rowgroup::SP_ROWAGG_t>&);
	static void prep1PhaseAggregate(
		JobInfo&, std::vector<rowgroup::RowGroup>&, std::vector<rowgroup::SP_ROWAGG_t>&);
	static void prep2PhasesAggregate(
		JobInfo&, std::vector<rowgroup::RowGroup>&, std::vector<rowgroup::SP_ROWAGG_t>&);
	static void prep2PhasesDistinctAggregate(
		JobInfo&, std::vector<rowgroup::RowGroup>&, std::vector<rowgroup::SP_ROWAGG_t>&);

	void prepExpressionOnAggregate(rowgroup::SP_ROWAGG_UM_t&, JobInfo&);
	void addConstangAggregate(std::vector<rowgroup::ConstantAggData>&);

	void doAggregate();
	void doAggregate_singleThread();
	uint64_t doThreadedAggregate(messageqcpp::ByteStream &bs, RowGroupDL* dlp);
	void aggregateRowGroups();
	void threadedAggregateRowGroups(uint8_t threadID);
	void doThreadedSecondPhaseAggregate(uint8_t threadID);
	bool nextDeliveredRowGroup();
	void pruneAuxColumns();
	void formatMiniStats();
	void printCalTrace();

	execplan::CalpontSystemCatalog *fCatalog;
	uint64_t fRowsReturned;
	bool     fDoneAggregate;
	bool     fEndOfResult;

	rowgroup::SP_ROWAGG_UM_t fAggregator;
	rowgroup::RowGroup fRowGroupOut;
	rowgroup::RowGroup fRowGroupDelivered;
	rowgroup::RGData fRowGroupData;

	// for setting aggregate column eid in delivered rowgroup
	//std::map<int, int> fIndexEidMap;

	// data from RowGroupDL
	rowgroup::RowGroup fRowGroupIn;

	// for PM HashJoin
	// PM hashjoin is selected at runtime, prepare for it anyway.
	rowgroup::SP_ROWAGG_UM_t fAggregatorUM;
	rowgroup::SP_ROWAGG_PM_t fAggregatorPM;
	rowgroup::RowGroup fRowGroupPMHJ;

	// for run thread (first added for union)
	class Aggregator
	{
	public:
		Aggregator(TupleAggregateStep* step) : fStep(step) { }
		void operator()() { fStep->doAggregate(); }

		TupleAggregateStep* fStep;
	};

	class ThreadedAggregator
	{
		public:
			ThreadedAggregator(TupleAggregateStep* step, uint8_t threadID) :
				fStep(step),
				fThreadID(threadID)
			{}
			void operator()() { fStep->threadedAggregateRowGroups(fThreadID); }

			TupleAggregateStep* fStep;
			uint8_t fThreadID;
	};

	class ThreadedSecondPhaseAggregator
	{
		public:
			ThreadedSecondPhaseAggregator(TupleAggregateStep* step, uint8_t threadID) :
				fStep(step),
				fThreadID(threadID)
			{
			}
			void operator()() {fStep->doThreadedSecondPhaseAggregate(fThreadID);}
			TupleAggregateStep* fStep;
			uint8_t fThreadID;
	};

	boost::scoped_ptr<boost::thread> fRunner;
	bool fUmOnly;
	ResourceManager& fRm;

	// multi-threaded
	uint fNumOfThreads;
	uint fNumOfBuckets;
	uint fNumOfRowGroups;
	uint64_t fBucketMask;
	uint fBucketNum;

	boost::mutex fMutex;
	std::vector<boost::mutex*> fAgg_mutex;
	std::vector<rowgroup::RGData > fRowGroupDatas;
	std::vector<rowgroup::SP_ROWAGG_UM_t> fAggregators;
	std::vector<rowgroup::RowGroup> fRowGroupIns;
	vector<rowgroup::RowGroup> fRowGroupOuts;
	std::vector<std::vector<rowgroup::RGData> > fRowGroupsDeliveredData;
	bool fIsMultiThread;
	int fInputIter; // iterator
	boost::scoped_array<uint64_t> fMemUsage;
};


} // namespace

#endif  // JOBLIST_TUPLEAGGREGATESTEP_H

// vim:ts=4 sw=4:
