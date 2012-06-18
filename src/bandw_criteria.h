
/*******************************************************/
/* CUDF solver: bandw_criteria.h                         */
/* Concrete class for the bandw criteria               */
/* (c) Arnaud Malapert I3S (UNSA-CNRS) 2012            */
/*******************************************************/


#ifndef _BANDW_CRITERIA_H_
#define _BANDW_CRITERIA_H_

#include <abstract_criteria.h>

// A concrete class for the bandw criteria
// i.e. the bandwidth allocated to connections.
class bandw_criteria: public pslp_criteria{
public:

	param_range stage_range;
	param_range length_range;

	bandw_criteria(CUDFcoefficient lambda_crit, int reliable, param_range stage_range, param_range length_range) : pslp_criteria(lambda_crit, reliable), stage_range(stage_range), length_range(length_range) {};
	virtual ~bandw_criteria() {}


	// Criteria initialization
	void initialize(PSLProblem *problem, abstract_solver *solver);

	// Allocate some columns for the criteria
	int set_variable_range(int first_free_var);
	// Add the criteria to the objective
	int add_criteria_to_objective(CUDFcoefficient lambda);
	// Add the criteria to the constraint set
	int add_criteria_to_constraint(CUDFcoefficient lambda);
	// Add constraints required by the criteria
	int add_constraints();

	int rank(pair<FacilityNode*, FacilityNode*> const &path, const unsigned int stage);

private :

	inline bool isRLSelected(pair<FacilityNode*, FacilityNode*> const &path) {
		if(length_range.contains(path.second->getType()->getLevel() - path.first->getType()->getLevel())) {
			return reliable == 0 ? ! isReliablePath(path.first, path.second) :
					reliable > 0 ? isReliablePath(path.first, path.second) : true;
		}
		return false;

	}

};

#endif

