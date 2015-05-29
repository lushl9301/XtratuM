#!/bin/bash


# ======================== Edit these three variables =================

read -e -p "1.- XtratuM installed base dir [${BASE_DIR}]: " U_BASE_DIR;
XTRATUM_TOOLCHAIN=/opt/sparc-linux-3.4.4/bin
read -e -p "2.- XTRATUM_TOOLCHAIN bin directory [${XTRATUM_TOOLCHAIN}]: " U_XTRATUM_TOOLCHAIN
RTEMS_PATH=${BASE_DIR}/rtems
read -e -p "3.- RTEMS installed dir [${RTEMS_PATH}]: " U_RTEMS_PATH;
RTEMS_TOOLCHAIN=/opt/rtems-4.8/bin
read -e -p "4.- RTEMS_TOOLCHAIN bin directory [${RTEMS_TOOLCHAIN}]: " U_RTEMS_TOOLCHAIN 


if [ -n "${U_BASE_DIR}" ]; then 
    BASE_DIR=$U_BASE_DIR;
fi

if [ -n "${U_RTEMS_PATH}" ]; then 
    RTEMS_PATH=$U_RTEMS_PATH;
fi

if [ -n "${U_RTEMS_TOOLCHAIN}" ]; then 
    RTEMS_TOOLCHAIN=$U_RTEMS_TOOLCHAIN;
fi

if [ -n "${U_XTRATUM_TOOLCHAIN}" ]; then 
    XTRATUM_TOOLCHAIN=$U_XTRATUM_TOOLCHAIN;
fi


# =========================  Cooked variables =========================
XTRATUM_PATH=$(echo ${BASE_DIR}/xm | sed sx//x/x)
XAL_PATH=$(echo ${BASE_DIR}/xal | sed sx//x/x)
LITHOS_PATH=$(echo ${BASE_DIR}/lithos | sed sx//x/x)
# =====================================================================




# ====================  Check that directoryes are correct  ===========

if ! test -x ${XTRATUM_TOOLCHAIN}/sparc-linux-gcc; then 
    echo "XTRATUM_TOOLCHAIN (${XTRATUM_TOOLCHAIN}) does not point to a valid directory."
    return;
fi

if test -f ${XTRATUM_PATH}/version; then 
    XMV=$(source  ${XTRATUM_PATH}/version ; echo $XTRATUMVERSION);
    XMV=XM-${XMV};
else
    echo "XTRATUM_PATH (${XTRATUM_PATH}) does not point to a valid directory."
    unset U_BASE_DIR  U_XTRATUM_TOOLCHAIN  U_RTEMS_TOOLCHAIN U_RTEMS_PATH BASE_DIR XTRATUM_PATH RTEMS_PATH RTEMSV RTEMS_TOOLCHAIN XTRATUM_TOOLCHAIN;
    return;
fi

# ======================= RTEMS  =====================
RTEMS_CPU_DEFS=${RTEMS_PATH}/sparc-rtems/leon2xm2/lib/include/rtems/score/cpuopts.h

if test -f ${RTEMS_CPU_DEFS}; then 
    if test -x ${RTEMS_TOOLCHAIN}/sparc-rtems-gcc; then 
	RTEMSV=$(grep VERSION ${RTEMS_CPU_DEFS} |  cut -f 3 -d" " | tr -d "\"");
	RTEMSV="|RTEMS-"${RTEMSV};
    fi
else
    echo "RTEMS: (${RTEMS_PATH}) does not point to a valid directory."
    unset RTEMS_PATH RTEMS_CPU_DEFS;
fi

# ======================= LITHOS =====================
if test -f ${LITHOS_PATH}/version; then 
    LTV=$(source  ${LITHOS_PATH}/version ; echo $LITHOSVERSION);
    LTV="|Lithos-"${LTV};
else 
    unset LITHOS_PATH;
fi

export XTRATUM_PATH XAL_PATH RTEMS_PATH LITHOS_PATH
export PATH=$PATH:${RTEMS_TOOLCHAIN}:${XTRATUM_TOOLCHAIN}


#echo -e "Note that your path shall start with: \033[00;36m(xm-$XMV:RTEMS-$RTEMSV:lvcugen)\033[00;0m"
#echo -e "Otherwise, you called this script in the wrong way.\n"

if  [ -n "${PS1_OLD}" ]; then 
    PS1=${PS1_OLD};
fi
PS1_OLD=${PS1}

export PS1="\[\033[00;36m\](${XMV}${RTEMSV}${LTV})\[\033[00;0m\]${PS1_OLD}"

unset U_BASE_DIR  U_XTRATUM_TOOLCHAIN  U_RTEMS_TOOLCHAIN U_RTEMS_PATH BASE_DIR XMV RTEMSV XTRATUM_TOOLCHAIN RTEMS_TOOLCHAIN LTV

function xmshowenv {
    if test -f ${XTRATUM_PATH}/version; then 
	XMV=$(source  ${XTRATUM_PATH}/version ; echo $XTRATUMVERSION);
    fi
    
    
    RTEMS_CPU_DEFS=${RTEMS_PATH}/sparc-rtems/leon2xm2/lib/include/rtems/score/cpuopts.h

    RTEMSV="-N/A-"
    if test -f ${RTEMS_CPU_DEFS}; then 
	RTEMSV=$(grep VERSION ${RTEMS_CPU_DEFS} |  cut -f 3 -d" " | tr -d "\"");
    fi

    LTV="-N/A-"
    if test -f ${LITHOS_PATH}/version; then 
	LTV=$(source  ${LITHOS_PATH}/version ; echo $LITHOSVERSION);
    fi

    printf "%-15s %-10s %s\n" "XTRATUM_PATH" ${XMV} ${XTRATUM_PATH}
    printf "%-15s %-10s %s\n" "XAL_PATH"      ${XMV} ${XAL_PATH}
    
    printf "%-15s %-10s %s\n" "RTEMS_PATH"    ${RTEMSV} ${RTEMS_PATH}
    printf "%-15s %-10s %s\n\n" "LITHOS_PATH"  ${LTV} ${LITHOS_PATH}

    unset LTV RTEMSV RTEMS_CPU_DEFS XMV

}