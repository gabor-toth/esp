import { ProgramDayType, ProgramDayValue, Program } from '../program/program';

export const simulatedPinConfiguration = {
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

export const simulatedPinState = {
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

export const simulatedPrograms: Program[] =
  [
    {
      enabled: true,
      days: {
        type: ProgramDayType.onDays,
        onDays: [
          ProgramDayValue.Mon,
          ProgramDayValue.Wed,
          ProgramDayValue.Fri
        ]
      },
      id: 1,
      lastRunTime: 0,
      name: "fű",
      nextRunTime: 0,
      startTimes: [
        "06:00",
        "18:00"
      ],
      zones: [
        {
          id: 1,
          duration: 600,
          name: "fű nagy"
        },
        {
          id: 2,
          duration: 300,
          name: "fű elől"
        },
        {
          id: 3,
          duration: 300,
          name: "fű hátul"
        }
      ]
    },
    {
      enabled: false,
      days: {
        type: ProgramDayType.onDays,
        onDays: [
          ProgramDayValue.Mon,
          ProgramDayValue.Wed,
          ProgramDayValue.Fri
        ]
      },
      id: 2,
      lastRunTime: 0,
      nextRunTime: 0,
      name: "veteményes",
      zones: [
        {
          id: 6,
          duration: 1800,
          name: "veteményes"
        }
      ],
      startTimes: [
        "06:00",
        "18:00"
      ]
    }
  ];

