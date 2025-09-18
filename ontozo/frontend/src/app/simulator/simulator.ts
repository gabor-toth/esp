import { Program, ProgramDay, ProgramDayType, ProgramDayValue, Programs, ProgramZone } from '../program/program';
import { PinsConfiguration, PinsState } from '../pin/pin';

export const simulatedPinConfiguration = <PinsConfiguration>{
  version: "1",
  inputs: {
    levels: [
      {
        id: 1,
        name: "Level 1"
      },
      {
        id: 2,
        name: "Level 2"
      },
      {
        id: 3,
        name: "Level 3"
      },
      {
        id: 4,
        name: "Level 4"
      }
    ]
  },
  outputs: {
    zones: [
      {
        id: 1,
        name: "fű nagy"
      },
      {
        id: 2,
        name: "fű elöl"
      },
      {
        id: 3,
        name: "fű hátul"
      },
      {
        id: 4,
        name: "kiskert elöl"
      },
      {
        id: 5,
        name: "kiskert hátul"
      },
      {
        id: 6,
        name: "veteményes"
      },
      {
        id: 7,
        name: "ribizli"
      }
    ],
    pumps: [
      {
        id: 1,
        name: "öntöző"
      },
      {
        id: 2,
        name: "kút"
      }
    ]

  }
}

export const simulatedPinState = <PinsState>{
  version: "1",
  inputs: {
    levels: [
      {
        id: 1,
        on: true
      },
      {
        id: 2,
        on: true
      },
      {
        id: 3,
        on: true
      },
      {
        id: 4,
        on: false
      }
    ]
  },
  outputs: {
    zones: [
      {
        id: 1,
        on: true
      },
      {
        id: 2,
        on: true
      },
      {
        id: 3,
        on: true
      },
      {
        id: 4,
        on: false
      },
      {
        id: 5,
        on: false
      },
      {
        id: 6,
        on: false
      },
      {
        id: 7,
        on: false
      }
    ],
    pumps: [
      {
        id: 1,
        on: false
      },
      {
        id: 2,
        on: true
      }
    ]

  }
}

export const simulatedPrograms: Programs =
  {
    version: "",
    programs:
      [
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
      ]
  };

