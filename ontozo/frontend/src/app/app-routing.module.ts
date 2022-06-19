import {NgModule} from '@angular/core';
import {RouterModule, Routes} from '@angular/router';
import {StateComponent} from "./state/state.component";
import {AdminComponent} from "./admin/admin.component";
import {ProgramComponent} from "./program/program.component";

const routes: Routes = [
    {path: '', redirectTo: '/dashboard', pathMatch: 'full'},
    {path: 'dashboard', component: StateComponent},
    // {path: 'detail/:id', component: HeroDetailComponent},
    {path: 'program', component: ProgramComponent},
    {path: 'admin', component: AdminComponent}
];

@NgModule({
    imports: [RouterModule.forRoot(routes)],
    exports: [RouterModule]
})
export class AppRoutingModule {
}
