import { Component, Inject, Input } from '@angular/core';
import {
  MAT_DIALOG_DATA,
  MatDialogActions,
  MatDialogClose,
  MatDialogContent,
  MatDialogRef,
  MatDialogTitle
} from "@angular/material/dialog";
import { MatButtonModule } from "@angular/material/button";

@Component( {
  selector: 'confirmation-dialog',
  templateUrl: 'confirmation.dialog.html',
  imports: [ MatDialogTitle, MatDialogContent, MatDialogActions, MatDialogClose, MatButtonModule ],
} )
export class ConfirmationDialog {
  constructor( protected dialogRef: MatDialogRef<ConfirmationDialog>,
               @Inject( MAT_DIALOG_DATA ) protected data: { confirmMessage: string } ) {
  }
}