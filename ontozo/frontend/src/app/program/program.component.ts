import { DataSource } from '@angular/cdk/collections';
import { Component, inject, OnInit, signal } from '@angular/core';
import { Program, ProgramDay, ProgramDayType, ProgramZone } from "./program";
import { ProgramService } from "./program.service";
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatCardModule } from "@angular/material/card";
import { MatButton } from "@angular/material/button";
import { SnackBar } from "../common/snackbar-error/snackbar";
import { ActivatedRoute, Router, RouterLink } from "@angular/router";
import { FormBuilder, FormControl, FormsModule, ReactiveFormsModule, Validators } from "@angular/forms";
import { MatError, MatFormField, MatInput, MatLabel } from "@angular/material/input";
import { MatCheckbox } from "@angular/material/checkbox";
import { MatChipInputEvent, MatChipsModule } from "@angular/material/chips";
import { MatRadioModule } from "@angular/material/radio";
import { MatOption, MatSelect } from "@angular/material/select";
import { MatListOption, MatSelectionList } from "@angular/material/list";
import { TimeSorter } from "../common/time.sorter";
import { CdkDrag, CdkDragDrop, CdkDragHandle, CdkDropList, moveItemInArray } from "@angular/cdk/drag-drop";
import { PinService } from "../pin/pin.service";
import { PinConfiguration } from "../pin/pin";
import {
  MatCell, MatCellDef,
  MatColumnDef,
  MatHeaderCell,
  MatHeaderCellDef,
  MatRow,
  MatRowDef,
  MatTable
} from "@angular/material/table";
import { MatRipple } from "@angular/material/core";
import { Observable, ReplaySubject } from "rxjs";

@Component( {
  selector: 'app-program',
  templateUrl: './program.component.html',
  styleUrls: [ './program.component.scss', './programs.component.scss' ],
  imports: [
    FormsModule,
    MatButton,
    MatCardModule,
    MatCheckbox,
    MatChipsModule,
    MatError,
    MatFormField,
    MatIcon,
    MatInput,
    MatLabel,
    MatListOption,
    MatOption,
    MatProgressSpinner,
    MatRadioModule,
    MatSelect,
    MatSelectionList,
    ReactiveFormsModule,
    RouterLink,
    CdkDrag,
    CdkDropList,
    MatTable,
    MatHeaderCell,
    MatCell,
    MatColumnDef,
    MatRow,
    MatRowDef,
    MatHeaderCellDef,
    MatCellDef,
    MatRipple,
    CdkDragHandle,
  ]
} )
export class ProgramComponent implements OnInit {
  readonly program = signal<Program | undefined>( undefined );
  readonly startTimes = signal<string[]>( [] );
  readonly programDaysDisplay: string[] = [ 'Hétfő', 'Kedd', 'Szerda', 'Csütörtök', 'Péntek', 'Szombat', 'Vasárnap' ];
  id: number | null = null;

  private readonly activatedRoute = inject( ActivatedRoute );
  private readonly formBuilder = inject( FormBuilder );
  private readonly pinService = inject( PinService );
  private readonly programService = inject( ProgramService );
  private readonly snackBar = inject( SnackBar );
  private readonly timeSorter = inject( TimeSorter );
  private readonly router = inject( Router );

  // workaround
  protected readonly ProgramDayType = ProgramDayType;

  nameFormControl = new FormControl( '', [ Validators.required ] );
  readonly chipFormControl = new FormControl( <string[]>[], [ Validators.required ] );
  readonly selectedProgramDayTypeControl = new FormControl( 0, [ Validators.required ] );
  readonly selectedDaysControl = new FormControl( 0, [ Validators.required ] );
  readonly intervalDaysControl = new FormControl( -1, [ Validators.required ] );
  readonly intervalStartsOnControl = new FormControl( -1, [ Validators.required ] );
  readonly zonesControl = new FormControl( -1, [ Validators.required ] );

  readonly fields = this.formBuilder.group( {
    active: false,
  } );

  protected readonly Array = Array;

  displayedColumns = [ 'reorder', 'name', 'duration', 'remove' ];

  intervalDays = signal<number | undefined>( 3 );
  intervalStartsOn = signal<number | undefined>( 1 );
  readonly selectedDays = signal<number[]>( [] );
  selectedProgramDayType = signal<ProgramDayType | undefined>( undefined );
  availableZones = signal<PinConfiguration[]>( [] );
  zones = signal<ProgramZoneData[]>( [] );
  zonesDataSource = new ProgramZoneDataSource( [] );

  constructor() {
    let id = this.activatedRoute.snapshot.params[ 'id' ];
    if ( id != null && id != "" ) {
      this.id = parseInt( id );
    }
  }

  ngOnInit(): void {
    this.updateState();
  }

  private updateState() {
    if ( this.id == null ) {
      return;
    }
    let component = this;
    this.pinService.getCachedConfiguration().subscribe( {
      next( zones ) {
        component.availableZones.set( zones.outputs.zones );
      },
      error( error ) {
        console.error( error );
      }
    } );
    if ( this.id == 0 ) {
      let program = {
        enabled: true,
        days: <ProgramDay>{
          type: ProgramDayType.unused,
        },
        id: 0,
        lastRunTime: 0,
        name: '',
        nextRunTime: 0,
        startTimes: [],
        zones: <ProgramZone[]>[]
      };
      this.onLoad( program );
    } else {
      let component = this;
      this.programService.get( this.id ).subscribe( {
        next( program ) {
          component.onLoad( program );
        },
        error( error ) {
          component.snackBar.open( 'Nem sikerült betölteni a programot.', error );
        },
      } );
    }
  }

  protected onLoad( program: Program ) {
    // set all fields every time!
    let component = this;
    this.program.set( program );
    this.nameFormControl.setValue( program.name );
    this.fields.controls.active.setValue( program.enabled );
    let programDayType = program.days.type;
    this.startTimes.set( program.startTimes );
    this.chipFormControl.setValue( program.startTimes );
    this.selectedProgramDayType.set( programDayType );
    this.selectedProgramDayTypeControl.setValue( programDayType );
    this.selectedDays.set( program.days.onDays || [] );
    this.intervalDays.set( program.days.intervalDays );
    this.intervalDaysControl.setValue( program.days.intervalDays! );
    this.intervalStartsOn.set( program.days.intervalStartsOn );
    this.intervalStartsOnControl.setValue( program.days.intervalStartsOn! );
    let zonesData = program.zones.map( _zone => component.toZoneData( _zone ) );
    this.zones.set( zonesData );
    this.zonesDataSource.setData( zonesData );
  }

  hasDay( dayIndex: number ): boolean {
    return this.selectedDays().includes( dayIndex );
  }

  addStartTime( event: MatChipInputEvent ): void {
    this.chipFormControl.setErrors( null );
    let value = ( event.value || '' ).trim();
    if ( !value ) {
      return;
    }

    let parts = value.split( ':' ).map( s => Number( s ) );
    if ( parts.length != 2
      || isNaN( parts[ 0 ] ) || isNaN( parts[ 1 ] )
      || parts[ 0 ] < 0 || parts[ 0 ] > 23
      || parts[ 1 ] < 0 || parts[ 1 ] > 59
    ) {
      this.chipFormControl.setErrors( { invalidTime: true } );
      return;
    }

    value = ( parts[ 0 ] >= 10 ? parts[ 0 ] : '0' + parts[ 0 ].toString() ) + ':' + ( parts[ 1 ] >= 10 ? parts[ 1 ] : '0' + parts[ 1 ].toString() );

    this.startTimes.update( keywords => this.timeSorter.sort( [ ...keywords, value ] ) );

    // Clear the input value
    event.chipInput!.clear();
  }

  removeStartTime( keyword: string ) {
    this.startTimes.update( keywords => {
      const index = keywords.indexOf( keyword );
      if ( index < 0 ) {
        return keywords;
      }

      keywords.splice( index, 1 );
      return [ ...keywords ];
    } );
  }

  protected onDayChange( dayIndex: number, hasDay: boolean ) {
    this.selectedDays.update( selectedDays => {
      if ( hasDay ) {
        selectedDays = selectedDays.filter( e => e != dayIndex );
      } else {
        selectedDays.push( dayIndex );
      }
      return selectedDays;
    } );
  }

  zoneDropped( event: CdkDragDrop<string> ) {
    moveItemInArray( this.zones(), event.previousIndex, event.currentIndex );
    this.zonesDataSource.setData( this.zones() );
  }

  zoneDeleted( index: number ) {
    //this.zones().filter( _zone => _zone != zone );
    this.zones().splice( index, 1 );
    this.zonesDataSource.setData( this.zones() );
  }

  protected zoneDurationChanged( zone: ProgramZone, minute: number ) {
    zone.duration = minute;
  }

  protected zoneIdChanged( zone: ProgramZone, selectedZone: PinConfiguration ) {
    zone.id = selectedZone.id;
    zone.name = selectedZone.name;
  }

  protected zoneAdded() {
    this.zones().push( this.toZoneData( <ProgramZone>{ id: 0, name: '', duration: 0 } ) );
    this.zonesDataSource.setData( this.zones() );
  }

  save(): void {
    let program = this.program();
    if ( program == undefined ) {
      return;
    }
    let component = this;

    let valid = true;
    if ( this.chipFormControl.invalid
      || this.nameFormControl.invalid
      || this.selectedProgramDayTypeControl.invalid
    ) {
      valid = false;
    }
    program.name = this.nameFormControl.getRawValue() || "";
    program.enabled = this.fields.controls.active.getRawValue() || false;
    program.days.type = this.selectedProgramDayType() || ProgramDayType.unused;
    program.startTimes = this.startTimes();
    if ( program.days.type == ProgramDayType.onDays ) {
      program.days.onDays = this.selectedDays() || [];
      if ( program.days.onDays.length == 0 ) {
        this.selectedDaysControl.setErrors( { required: true } );
        valid = false;
      }
    } else if ( program.days.type == ProgramDayType.interval ) {
      program.days.intervalDays = this.intervalDays() || 0;
      if ( program.days.intervalDays == 0 ) {
        this.intervalDaysControl.setErrors( { required: true } );
        valid = false;
      }
      program.days.intervalStartsOn = this.intervalStartsOn() || -1;
      if ( program.days.intervalStartsOn == -1 ) {
        this.intervalStartsOnControl.setErrors( { required: true } );
        valid = false;
      }
    } else {
      this.selectedProgramDayTypeControl.setErrors( { required: true } );
      valid = false;
    }
    if ( this.zones().length == 0 ) {
      this.zonesControl.setErrors( { required: true } );
      valid = false;
    } else {
      this.zonesControl.setErrors( null );
    }
    this.zones().forEach( function ( zone ) {
      console.log( zone );
      if ( zone.id == 0 ) {
        zone.nameControl.setErrors( { required: true } );
        valid = false;
      }
      if ( zone.duration == 0 ) {
        zone.durationControl.setErrors( { required: true } );
        valid = false;
      }
    } );
    program.zones = this.zones().map( zoneData => component.fromZoneData( zoneData ) );

    if ( !valid ) {
      return;
    }

    this.programService.set( program ).subscribe( {
      next( program ) {
        component.router.navigate( [ '/programs' ] );
        component.snackBar.message( 'Program sikeresen mentve.' );
      },
      error( error ) {
        component.snackBar.open( 'Nem sikerült menteni a programot', error );
      },
    } );
  }

  private toZoneData( zone: ProgramZone ): ProgramZoneData {
    return {
      ...zone,
      nameControl: new FormControl( zone.id, [ Validators.required ] ),
      durationControl: new FormControl( zone.duration, [ Validators.required ] ),
    };
  }

  private fromZoneData( zoneData: ProgramZoneData ): ProgramZone {
    return {
      duration: zoneData.duration,
      id: zoneData.id,
      name: zoneData.name,
    };
  }
}

class ProgramZoneDataSource extends DataSource<ProgramZoneData> {
  private _dataStream = new ReplaySubject<ProgramZoneData[]>();

  constructor( initialData: ProgramZoneData[] ) {
    super();
    this.setData( initialData );
  }

  connect(): Observable<ProgramZoneData[]> {
    return this._dataStream;
  }

  disconnect() {
  }

  setData( data: ProgramZoneData[] ) {
    this._dataStream.next( data );
  }
}

interface ProgramZoneData extends ProgramZone {
  nameControl: FormControl;
  durationControl: FormControl;
}
