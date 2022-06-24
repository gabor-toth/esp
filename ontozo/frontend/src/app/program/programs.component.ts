import {Component, OnInit} from '@angular/core';
import {Program, ProgramHeader} from "./program";
import {MatTableDataSource, MatTableModule} from '@angular/material/table';
import {ProgramService} from "./program.service";
import {faToggleOff, faToggleOn} from '@fortawesome/free-solid-svg-icons';
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
    faToggleOff = faToggleOff
    faToggleOn = faToggleOn

    displayedColumns: string[] = ['name', 'enabled', 'expand'];
    dataSource: MatTableDataSource<ProgramHeader>;
    programs: ProgramHeader[] | undefined;
    expandedElement: ProgramHeader | null;

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

    clickExpand(element: ProgramHeader) {
        let component = this;
        this.expandedElement = this.expandedElement === element ? null : element;
        if (element.program == null) {
            this.programService.get(element.index).subscribe({
                next(program) {
                    element.program = program;
                },
                error(err) {
                    console.error('Error reading program ' + element.index, err);
                },
            });
        }
    }

    startProgram(element: ProgramHeader) {
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
}
