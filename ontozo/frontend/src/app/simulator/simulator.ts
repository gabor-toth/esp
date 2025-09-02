import { Program, ProgramDay, ProgramDayType, ProgramDayValue, ProgramZone } from '../program/program';
import { PinsState } from '../pin/pin';

export const simulatedPinState = <PinsState>{
  inputs: {
    levels: [
      {
        id: 1,
        name: "Level 1",
        on: true
      },
      {
        id: 2,
        name: "Level 2",
        on: true
      },
      {
        id: 3,
        name: "Level 3",
        on: true
      },
      {
        id: 4,
        name: "Level 4",
        on: false
      }
    ]
  },
  outputs: {
    zones: [
      {
        id: 1,
        name: "fű nagy",
        on: true
      },
      {
        id: 2,
        name: "fű elöl",
        on: true
      },
      {
        id: 3,
        name: "fű hátul",
        on: true
      },
      {
        id: 4,
        name: "kiskert elöl",
        on: false
      },
      {
        id: 5,
        name: "kiskert hátul",
        on: false
      },
      {
        id: 6,
        name: "veteményes",
        on: false
      },
      {
        id: 7,
        name: "ribizli",
        on: false
      }
    ],
    pumps: [
      {
        id: 1,
        name: "öntöző",
        on: false
      },
      {
        id: 2,
        name: "kút",
        on: true
      }
    ]

  }
}

export const simulatedPrograms: Program[] = [
  {
    enabled: true,
    days: <ProgramDay>{
      type: <ProgramDayType><unknown>ProgramDayType[ ProgramDayType.onDays ],
      onDays: <ProgramDayValue[]><unknown>[
        ProgramDayValue[ ProgramDayValue.Mon ],
        ProgramDayValue[ ProgramDayValue.Wed ],
        ProgramDayValue[ ProgramDayValue.Fri ]
      ]
    },
    index: 1,
    lastRunTime: 0,
    name: "fű",
    nextRunTime: 0,
    startTimes: [
      "06:00",
      "18:00"
    ],
    valid: true,
    zones: <ProgramZone[]>[
      <ProgramZone>{
        zoneId: 1,
        duration: 600
      },
      <ProgramZone>{
        zoneId: 2,
        duration: 300
      },
      <ProgramZone>{
        zoneId: 3,
        duration: 300
      }
    ]
  },
  <Program>{
    enabled: false,
    days: <ProgramDay>{
      type: <ProgramDayType><unknown>ProgramDayType[ ProgramDayType.onDays ],
      onDays: <ProgramDayValue[]><unknown>[
        ProgramDayValue[ ProgramDayValue.Mon ],
        ProgramDayValue[ ProgramDayValue.Wed ],
        ProgramDayValue[ ProgramDayValue.Fri ]
      ]
    },
    index: 2,
    name: "veteményes",
    zones: <ProgramZone[]>[
      <ProgramZone>{
        zoneId: 5,
        duration: 1800
      }
    ],
    valid: true,
  }
];

