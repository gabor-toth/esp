import { Component, OnInit, Inject } from '@angular/core';
import { MAT_SNACK_BAR_DATA, MatSnackBarRef } from "@angular/material/snack-bar";
import { MatIcon } from '@angular/material/icon';
import { MatButton } from '@angular/material/button';
import { SnackbarComponent, SnackBarType } from "./snackbar.component";

@Component( {
  selector: 'app-snackbar-message',
  templateUrl: './snackbar.component.html',
  imports: [
    MatIcon,
    MatButton
  ],
  styleUrls: [ './snackbar.component.css' ]
} )
export class SnackbarMessageComponent extends SnackbarComponent {

  constructor( @Inject( MAT_SNACK_BAR_DATA ) public override message: any ) {
    super( message, SnackBarType.message );
  }

}
