import { Component, OnInit, Inject } from '@angular/core';
import { MAT_SNACK_BAR_DATA, MatSnackBarRef } from "@angular/material/snack-bar";
import {MatIcon} from '@angular/material/icon';
import {MatButton} from '@angular/material/button';

@Component({
  selector: 'app-snackbar-error',
  templateUrl: './snackbar-error.component.html',
  imports: [
    MatIcon,
    MatButton
  ],
  styleUrls: ['./snackbar-error.component.css']
})
export class SnackbarErrorComponent implements OnInit {

  constructor( private matDialogRef: MatSnackBarRef<SnackbarErrorComponent>, @Inject( MAT_SNACK_BAR_DATA ) public message: any ) {
  }

  ngOnInit() {
  }

  close() {
    this.matDialogRef.dismiss();
  }

}
