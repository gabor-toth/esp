import { Component, inject, Inject, Input, Service } from '@angular/core';
import {
  MAT_DIALOG_DATA, MatDialog,
  MatDialogActions,
  MatDialogClose,
  MatDialogContent,
  MatDialogRef,
  MatDialogTitle
} from "@angular/material/dialog";
import { MatButtonModule } from "@angular/material/button";
import { ConfirmationDialog } from "./confirmation.dialog";

@Service( {} )
export class Confirmation {
  private readonly dialog = inject( MatDialog );

  constructor() {
  }


  public open( confirmMessage: string ) {
    const dialogRef = this.dialog.open( ConfirmationDialog, { data: { 'confirmMessage': confirmMessage } } );
    dialogRef.afterClosed().subscribe( result => {
    } );
  }
}