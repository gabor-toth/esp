import { Injectable } from "@angular/core";
import { MatSnackBar } from "@angular/material/snack-bar";
import { SnackbarErrorComponent } from "./snackbar-error.component";
import { SnackbarMessageComponent } from "./snackbar-message.component";

@Injectable( {
  providedIn: 'root'
} )
export class SnackBar {
  constructor( private snackBar: MatSnackBar ) {
  }

  open( log: string, error: any, message?: string ) {
    console.error( log, error );
    this.snackBar.openFromComponent(
      SnackbarErrorComponent,
      {
        data: message != undefined ? message : 'Hiba a kapcsolatban.',
        duration: 10000,
        horizontalPosition: 'right',
        panelClass: 'error-snackbar',
        verticalPosition: 'bottom',
      } );
  }

  message( message: string ) {
    this.snackBar.openFromComponent(
      SnackbarMessageComponent,
      {
        data: message != undefined ? message : 'Hiba a kapcsolatban.',
        duration: 10000,
        horizontalPosition: 'right',
        panelClass: 'error-snackbar',
        verticalPosition: 'bottom',
      } );
  }

}