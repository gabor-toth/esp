import { Component, OnInit, Inject, inject } from '@angular/core';
import { MAT_SNACK_BAR_DATA, MatSnackBarRef } from "@angular/material/snack-bar";
import { MatIcon } from '@angular/material/icon';
import { MatButton } from '@angular/material/button';

export enum SnackBarType {
  message,
  error,
}

@Component( {
  //selector: 'app-snackbar-error',
  templateUrl: './snackbar.component.html',
  imports: [
    MatIcon,
    MatButton
  ],
  styleUrls: [ './snackbar.component.css' ]
} )
export class SnackbarComponent implements OnInit {
  private matDialogRef = inject( MatSnackBarRef<SnackbarComponent> );
  public readonly icon: string;
  public readonly cssClass: string;

  constructor( @Inject( MAT_SNACK_BAR_DATA ) public message: any, type: SnackBarType ) {
    switch ( type ) {
      case SnackBarType.message:
        this.icon = "check";
        this.cssClass = "snackbar-success-message";
        break;
      default:
        this.icon = "warning";
        this.cssClass = "snackbar-error-message";
        break;
    }
  }

  ngOnInit() {
  }

  close() {
    this.matDialogRef.dismiss();
  }

}
