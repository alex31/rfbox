#include <ch.h>
#include <hal.h>
#include "stdutil.h"	

#include "notGate.hpp"

namespace {
  static const COMPConfig comp1Cfg = {
    .output_inverted = true,
    .csr = COMP_CSR_COMPxPOL |		
    COMP_CSR_COMP1INP_PA01 |
    COMP_CSR_COMPxINM_1VREF_DIV_2 |	// connection interne entre COMP1_IN_MOINS et 1/2 VREF
    COMP_CSR_COMP1_BLANKING_NOBLANK 	// pas de blanking
  };
}

void notGateStart(void)
{
  compStart(&COMPD1, &comp1Cfg);
  compEnable(&COMPD1);
}
    
