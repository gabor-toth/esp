import {Component, OnInit} from '@angular/core';
import {Program, ProgramDayType, ProgramDayValue} from "./program";
import {MatTableDataSource, MatTableModule} from '@angular/material/table';
import {ProgramService} from "./program.service";
import {animate, state, style, transition, trigger} from '@angular/animations';
import {RunService} from "./run.service";

@Component({
    selector: 'app-program',
    templateUrl: './programs.component.html',
    styleUrls: ['./programs.component.scss'],
    animations: [
        trigger('detailExpand', [
            state('collapsed', style({height: '0px', minHeight: '0'})),
            state('expanded', style({height: '*'})),
            transition('expanded <=> collapsed', animate('225ms cubic-bezier(0.4, 0.0, 0.2, 1)')),
        ]),
    ],
})
export class ProgramsComponent implements OnInit {
    displayedColumns: string[] = ['name', 'enabled', 'start', 'expand'];
    displayedColumnsMobile: string[] = ['name', 'enabled', 'expand'];
    dataSource: MatTableDataSource<Program>;
    programs: Program[] | undefined;
    expandedElement: Program | null;
    programDaysDisplay: string[] = ['H', 'K', 'Sz', 'Cs', 'P', 'Sz', 'V'];
    readonly ProgramDayType = ProgramDayType;

    constructor(private programService: ProgramService, private runService: RunService) {
        this.dataSource = new MatTableDataSource(this.programs);
        this.expandedElement = null;
    }

    ngOnInit(): void {
        this.updateState();
    }

    private updateState() {
        let component = this;
        this.programService.getAll().subscribe({
            next(programs) {
                component.programs = programs;
                component.dataSource = new MatTableDataSource(component.programs);
            },
            error(err) {
                console.error('Error reading programs', err);
            },
        });
    }

    clickExpand(element: Program) {
        let component = this;
        this.expandedElement = this.expandedElement === element ? null : element;
    }

    startProgram(element: Program) {
        this.runService.start(element.index).subscribe({
            next(dummy) {
                // toaster: add success
            },
            error(err) {
                // toaster: add error
                console.error('Error starting program ' + element.index, err);
            },
        });
    }

    stopProgram() {
        this.runService.stop().subscribe({
            next(dummy) {
            },
            error(err) {
                console.error('Error stopping program', err);
            },
        });
    }

    nextZone() {
        this.runService.nextZone().subscribe({
            next(dummy) {
            },
            error(err) {
                console.error('Error moving to next zone', err);
            },
        });
    }

    hasDay(program: Program, dayIndex: number): boolean {
        return program.days.onDays.find(e => String(e.valueOf()) == ProgramDayValue[dayIndex]) != null;
    }
}
