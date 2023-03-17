#!/bin/bash

pcbname=$(basename $(pwd))
outdir=gerber

function rename_gerber() {
  local suffix=$1
  local ext=$2
  
  local src=${outdir}/${pcbname}-${suffix}.*
  
  if [[ -n $ext ]]; then
    mv $src ${outdir}/${pcbname}.${ext}
  else
    rm $src
  fi
}

rename_gerber F_Cu		GTL
rename_gerber B_Cu		GBL
rename_gerber B_Paste		""
rename_gerber F_Paste		""
rename_gerber F_Mask		GTS
rename_gerber B_Mask		GBS
rename_gerber F_Silkscreen	GTO
rename_gerber B_Silkscreen	GBO
rename_gerber PTH		TXT
rename_gerber NPTH		XLN
rename_gerber Edge_Cuts		GKO # GML

rm ${outdir}/${pcbname}-job.gbrjob

cd ${outdir}
outzip=../${pcbname}.zip
rm -f $outzip
zip -q $outzip *

