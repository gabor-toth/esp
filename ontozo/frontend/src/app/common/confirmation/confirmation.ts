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
import { Observable } from "rxjs";

@Service( {} )
export class Confirmation {
  private readonly dialog = inject( MatDialog );

  constructor() {
  }


  public open( confirmMessage: string ) {
    const dialogRef = this.dialog.open( ConfirmationDialog, { data: { 'confirmMessage': confirmMessage } } );
    return new Observable<boolean>( ( subscriber ) => {
      dialogRef.afterClosed().subscribe( result => {
        if ( result ) {
          subscriber.next( true );
        }
        subscriber.complete();
      } );
    } );
  }
}