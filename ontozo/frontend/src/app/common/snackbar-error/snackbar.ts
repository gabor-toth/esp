import { Injectable } from "@angular/core";
import { MatSnackBar } from "@angular/material/snack-bar";
import { SnackbarErrorComponent } from "./snackbar-error.component";

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

}